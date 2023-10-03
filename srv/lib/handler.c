#include "h/handler.h"
#include "h/http_response.h"
#include "h/http_request.h"
#include "h/picohttpparser.h"
#include "h/connection_context.h"
#include "h/utils.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <strings.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

http_response* build_response(connection_context* context){

    http_response* res = http_response_create(200, "Content-Type: text/html; charset=utf-8\n", "<h1>Hello from <i>pronto</i></h1>", context->fd);

    /*
        we are ready to stream the response to the socket! 
        to actually stream something we need to save the socket's fd (check comment 1 in h/http_response.h)
        so that when we are back in the epoll loop we have a valid file descriptor to stream to 
        (we can't use both the "fd" and the "ptr" attributes when using epolls)
    */
    // context_destroy(context);
    return res;

}

void *handler_process_request(void *h){

    handler *current_handler = (handler *)h;
    // let's keep the request buffer on the stack so it's faster to be read from
    char buf[current_handler->request_buffer_size];
    bzero(buf, current_handler->request_buffer_size);
    current_handler->request_buffer = buf;

    while (current_handler->active){

        int ready_events = epoll_wait(current_handler->epoll_fd, current_handler->events, current_handler->max_events, -1);
        for (int i = 0; i < ready_events; i++){

            uint32_t events = current_handler->events[i].events;

            if (events == EPOLLIN){
               
                int broken_request = 0, request_too_large = 0;
                size_t request_size = 0;

                connection_context* ctx = malloc(sizeof(connection_context));
                context_init(ctx, current_handler->request_buffer_size);
                ctx->fd = current_handler->events[i].data.fd;

                char buf[256];
                char *method, *path;
                int parse_result, minor_version;
                struct phr_header headers[100];
                size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
                ssize_t read_bytes;

                while(1){
                    bzero(buf, sizeof(buf));
                    while ((read_bytes = read(ctx->fd, buf, sizeof(buf))) == -1 && errno == EINTR);
                    if(errno == EWOULDBLOCK || errno == EAGAIN){
                        broken_request = 1;
                        break; // we suppose the caller hung up 
                        // in a more forgiving environment, this would mean 
                        // that the socket isn't ready yet, we should 
                        // queue the descriptor for later (rearm epoll and wait for it to signal stuff)
                    }
                    prevbuflen = buflen;
                    buflen += read_bytes;
                    context_write(ctx, buf, read_bytes);
                    if(read_bytes <= 0){
                        broken_request = 1;
                        break; // refactor in while
                    }
                    request_size += read_bytes;
                    if(request_size > current_handler->max_request_size){
                        request_too_large = 1;
                        break;
                    }
                    num_headers = sizeof(headers) / sizeof(headers[0]);
                    parse_result = phr_parse_request(
                        ctx->data, 
                        buflen, 
                        (const char **) &method, 
                        &method_len, 
                        (const char **) &path, 
                        &path_len,
                        &minor_version, 
                        headers, 
                        &num_headers, 
                        prevbuflen
                    );
                    if (parse_result > 0)
                        break; // request parsed correctly
                    else if (parse_result == -1){
                        broken_request = 1;
                        break;
                    }
                    if(parse_result != -2){
                        printf("BUG_ON: parse_result is not -2 while request is being processed");
                        broken_request = 1;
                        break;
                    }
                    
                }

                ctx->data[ctx->length] = 0;

                if(broken_request == 1){
                    epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_DEL, current_handler->events[i].data.fd, NULL);
                    close(current_handler->events[i].data.fd);
                    // context_destroy(ctx);
                    continue; // skip the request
                }

                if(request_too_large == 1){
                    // send 413
                    epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_DEL, current_handler->events[i].data.fd, NULL);
                    close(current_handler->events[i].data.fd);
                    // context_destroy(ctx);
                    continue;
                }
                
                http_request* req = malloc(sizeof(http_request));
                http_request_init(
                    req,
                    method,
                    method_len,
                    path,
                    path_len,
                    headers,
                    num_headers
                );

                if(
                    strcmp(req->method, "POST") == 0 
                    || strcmp(req->method, "post") == 0
                ){
                    // body starts at req_buffer + parse_result
                    // https://github.com/h2o/picohttpparser/issues/59
                    req->body_len = (strlen(ctx->data) - parse_result);
                    req->body = malloc(sizeof(char) * req->body_len); // null-terminate
                    bzero((void*) req->body, req->body_len);
                    strncpy((char*) req->body, ctx->data + parse_result, req->body_len);
                    req->json_body = cJSON_ParseWithLength(req->body, req->body_len);
                    /*char* json = cJSON_Print(req->json_body);
                    printf("%s\n", json);*/
                }

                http_response *res = build_response(ctx);
                // http_request_destroy(req);

                struct epoll_event add_write_event;
                add_write_event.events = EPOLLOUT;
                add_write_event.data.ptr = res;

                /*
                    on the same file descriptor we can attach everything, we can also listen to changes on a certain
                    data structure!
                    that's what we did with the "ptr" attribute of the epoll_event:
                    we put our response there so, when there is data being written on that data structure, the epoll will trigger
                    a new event!
                    we will write data as soon as we process the request, so the epoll will trigger instantly.

                    this method makes us able to write the response chunk by chunk (check (**) down below), without blocking other requests in the loop!
                */

                if (epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_MOD, current_handler->events[i].data.fd, &add_write_event) < 0){
                    perror("cannot set epoll descriptor ready for writing\n");
                    close(current_handler->events[i].data.fd);
                    // http_response_destroy(res);
                    // context_destroy(ctx);
                    continue;
                }

            }else if (events == EPOLLOUT){

                /*
                    (**) here we are, consuming what we wrote (an http response) on the epoll!
                    this part of the loop enables us, as already mentioned, to stream the http response to the client without blocking
                    other input or output streams!
                    with the use of the "stream_ptr" variable (check lib/h/http_response.h, comment 2) we can efficiently keep track
                    of the chunk we are writing to the client, incrementing it whenever we write some bytes!

                    note how we write data: we use the streamed_response->stringified buffer + the stream_ptr (basically an offset),
                    because in this way we keep track of what we already wrote (by summing stream_ptr).
                */

                http_response *streamed_response = (http_response *) current_handler->events[i].data.ptr;
                int written_bytes = send(
                    streamed_response->socket,
                    streamed_response->stringified + streamed_response->stream_ptr,
                    streamed_response->full_length,
                    0
                );
                if (written_bytes >= 0){
                    streamed_response->stream_ptr += written_bytes; // here we update our pointer!
                    if (streamed_response->stream_ptr >= streamed_response->full_length){
                        /*
                            we wrote everything, so we need to close the connection (only HTTP/1.1)
                        */
                        if (epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_DEL, streamed_response->socket, NULL) < 0){
                            perror("cannot close epoll fd\n");
                        }
                        close(streamed_response->socket);
                        // http_response_destroy(streamed_response);
                    }
                }else if (written_bytes == -1){
                    if (errno == EAGAIN || errno == EWOULDBLOCK){
                        // the socket is not available yet
                        continue;
                    }
                    else{
                        // http_response_destroy(streamed_response);
                        perror("send failed\n");
                    }
                }
            }else{
                /*
                    something bad happened to our client, let's ignore its request
                */
                if (epoll_ctl(current_handler->epoll_fd, EPOLL_CTL_DEL, current_handler->events[i].data.fd, NULL) < 0){
                    perror("cannot close epoll fd\n");
                }
                close(current_handler->events[i].data.fd);
            }
        }
    }

    pthread_exit(0);
}

void handler_init(handler *handler, int max_events, int buf_size, int max_request_size){

    handler->thread = (pthread_t *)malloc(sizeof(pthread_t));
    handler->active = true;
    handler->max_events = max_events;
    handler->epoll_fd = epoll_create1(0);
    handler->request_buffer_size = buf_size;
    handler->max_request_size = max_request_size;
    handler->events = malloc(sizeof(struct epoll_event) * max_events);

    printf("handler config:\n");
    printf("\t request_buffer_size: %d\n", handler->request_buffer_size);

    pthread_create(handler->thread, NULL, handler_process_request, (void *)handler);

}