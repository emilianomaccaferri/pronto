# for some reason i didnt debug yet, the compilation doesnt work on docker
# so, for now, it needs the prebuilt binary 
FROM ubuntu:latest as run
WORKDIR /app
COPY ./bin/pronto-srv /app
RUN apt update
RUN apt install -y curl

