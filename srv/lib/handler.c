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

void build_response(
    http_response* res,
    http_request* req,
    int is_bad_request,
    int socket
){

    if(req->json_body)
        printf("%s\n", cJSON_Print(req->json_body));
    
    if(is_bad_request == 1){
        // send bad request
        memcpy(res, http_response_bad_request(socket), sizeof(http_response));
    }else{
        if(strcmp(req->path, "/schedule") == 0){
            // do the schedule
            memcpy(
                res, http_response_create(
                    200,
                    "Content-Type: application/json",
                    "{\"success\": true, \"hello\": [1,2,3,4]}",
                    socket
                ), 
                sizeof(http_response)
            );
        }else{
            memcpy(res, http_response_not_found(socket), sizeof(http_response));
        }
        
    }

    free(req->method);
    free(req->headers);

}

void *handler_process_request(void *h){

    handler *current_handler = (handler *)h;
    // let's keep the request buffer on the stack so it's faster to be read from
    /*char buf[current_handler->request_buffer_size];
    bzero(buf, current_handler->request_buffer_size);
    current_handler->request_buffer = buf;*/

    while (current_handler->active){

        int ready_events = epoll_wait(
            current_handler->epoll_fd, 
            current_handler->events, 
            current_handler->max_events,
            -1
        );
        for (int i = 0; i < ready_events; i++){

            uint32_t events = current_handler->events[i].events;
            char request_buffer[current_handler->max_request_size]; // we read the request in a 128k buffer allocated in the stack

            if (events == EPOLLIN){
               
                int broken_request = 0, 
                    request_too_large = 0, 
                    with_body = 0,
                    is_bad_request = 0,
                    retries = 0;
                size_t request_size = 0;

                // connection_context* ctx = malloc(sizeof(connection_context));
                // context_init(ctx, current_handler->request_buffer_size);
                // ctx->fd = current_handler->events[i].data.fd;
                char *method, *path;
                int request_length, minor_version; // request_length will represent the length of the HTTP request
                                                   // without the body
                                                   // the body will start at request_buffer + request_length
                struct phr_header headers[100];
                size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
                ssize_t read_bytes;
                http_request incoming_req;
                errno = 0;

                while(1){
                    // we read [request_buffer_size] bytes and we put them in the "big" buffer (request_buffer)
                    // that represents the whole request
                    while (
                        (read_bytes = read(
                            current_handler->events[i].data.fd, 
                            request_buffer + buflen, 
                            current_handler->request_buffer_size
                        )
                    ) == -1 && errno == EINTR); 
                    // 1. read from the socket (current_handler->events[i].data.fd),
                    // 2. put what has been read inside the "big" buffer + an offset: the offset represents how much has been read until now
                    // 3. read [request_buffer_size] bytes each time!
                    
                    if(errno == EWOULDBLOCK || errno == EAGAIN){
                        retries++;
                        if(retries >= 5){
                            break; // the request has been drained
                        }
                        continue; // we wait for the socket in case it got stuck or something
                    }
                    prevbuflen = buflen;
                    buflen += read_bytes;
                    // context_write(ctx, buf, read_bytes);
                    if(read_bytes <= 0){
                        broken_request = 1;
                        break; // refactor in while
                    }
                    request_size += read_bytes;
                    if(request_size > current_handler->max_request_size){
                        request_too_large = 1;
                        break;
                    }
                    if(with_body == 1){
                        continue; // stop parsing the request, we need the body
                    }
                    num_headers = sizeof(headers) / sizeof(headers[0]);
                    request_length = phr_parse_request(
                        request_buffer, 
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
                    if (request_length > 0){
                        // the request (NOT THE BODY) has been
                        // successfully parsed
                        // we have now information about stuff that we can use to parse the body
                        // method and path are just pointers inside the request_buffer!

                        http_request_from(
                            &incoming_req,
                            method,
                            path,
                            headers,
                            method_len,
                            path_len,
                            num_headers
                        );
                        
                        // printf("whole request: %s\n", request_buffer);
                        if(
                            strcmp(incoming_req.method, "POST") == 0 
                            || strcmp(incoming_req.method, "post") == 0
                        ){
                            // drain the request because we need the body
                            with_body = 1;
                            continue; // skip the parsing!
                        }
                    }
                    else if (request_length == -1){
                        broken_request = 1;
                        break;
                    }
                    if(request_length != -2){
                        printf("BUG_ON: parse_result is not -2 while request is being processed");
                        broken_request = 1;
                        break;
                    }
                    
                }

                if(broken_request == 1){
                    is_bad_request = 1;
                    continue; // skip the request
                }

                if(request_too_large == 1){
                    is_bad_request = 1;
                    continue;
                }

                // we now have the full request inside the request_buffer
                // printf("%s\n", request_buffer);
                if(with_body){
                    // parse the body!
                    // TODO: refactor into a function
                    char* str_content_length = get_header(
                        incoming_req.headers, 
                        incoming_req.num_headers, 
                        "Content-Length"
                    );
                    if(str_content_length == NULL){
                        is_bad_request = 1;
                    }
                    int content_length = atoi(str_content_length);
                    free(str_content_length);
                    // body starts at req_buffer + phr_parse_result
                    // https://github.com/h2o/picohttpparser/issues/59
                    char body[content_length + 1];
                    strncpy(body, request_buffer + request_length, content_length);
                    body[content_length] = 0; // 0-terminate again!
                    incoming_req.body = body;
                    incoming_req.json_body = cJSON_ParseWithLengthOpts(
                        body,
                        content_length + 1,
                        NULL,
                        1
                    );
                    if(incoming_req.json_body == NULL){
                        is_bad_request = 1;
                    }
                }

                // THE PARSING OF THE REQUEST IS COMPLETE (finally)
                // we have to create a response based on the request!
                http_response *res = malloc(sizeof(http_response));
                build_response(res, &incoming_req, is_bad_request, current_handler->events[i].data.fd);
                /*
                    we are ready to stream the response to the socket! 
                    to actually stream something we need to save the socket's fd (check comment 1 in h/http_response.h)
                    so that when we are back in the epoll loop we have a valid file descriptor to stream to 
                    (we can't use both the "fd" and the "ptr" attributes when using epolls)
                */

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
                    http_response_destroy(res);
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
                        http_response_destroy(streamed_response);
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

void handler_init(
    handler *handler, 
    int max_events, 
    int buf_size, 
    int max_request_size,
    int max_body_size
){

    handler->thread = (pthread_t *)malloc(sizeof(pthread_t));
    handler->active = true;
    handler->max_events = max_events;
    handler->epoll_fd = epoll_create1(0);
    handler->request_buffer_size = buf_size;
    handler->max_body_size = max_body_size;
    handler->max_request_size = max_request_size;
    handler->events = malloc(sizeof(struct epoll_event) * max_events);

    printf("handler config:\n");
    printf("\t request_buffer_size: %d\n", handler->request_buffer_size);
    printf("\t max_body_size: %d\n", handler->max_body_size);

    pthread_create(handler->thread, NULL, handler_process_request, (void *)handler);

}