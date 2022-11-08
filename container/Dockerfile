# recommend not to use the alpine one, it lacks lots of dependencies
# the slim one ocuppies about 2x space compared to alpine one
# FROM python:3.7-alpine
FROM rust:1.64

# fulfill the structure requirement of proxy
RUN mkdir /src

# copy libs
COPY proxy.rs /src/main.rs
COPY main.wat /src/main.wat
COPY Cargo.toml /Cargo.toml

WORKDIR /

# proxy server runs under port 8000
EXPOSE 8000

# for alpine base only
# RUN apk update && \
#     apk add --no-cache --virtual .build-deps gcc musl-dev libffi-dev make && \
#     pip install --no-cache-dir gevent flask && \
#     apk del .build-deps

RUN cargo build

CMD ["cargo", "run"]