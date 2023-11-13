FROM rust:1.73 as build
WORKDIR /code
COPY ./src /code/src
COPY ./Cargo.toml /code/Cargo.toml
RUN cargo build --release

# /code/target/release/pronto-client

FROM ubuntu:latest as run
WORKDIR /app
COPY --from=build /code/target/release/pronto-client /app
