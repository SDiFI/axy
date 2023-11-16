FROM debian:bookworm-slim AS build

ARG VERSION=0.15.0

RUN apt-get update -qq && apt-get install -yqq curl ca-certificates unzip && rm -rf /var/lib/apt/lists/*
RUN curl -L https://github.com/improbable-eng/grpc-web/releases/download/v$VERSION/grpcwebproxy-v$VERSION-linux-x86_64.zip -o grpcwebproxy.zip && unzip grpcwebproxy.zip && rm grpcwebproxy.zip
RUN cp dist/grpcwebproxy-v$VERSION-linux-x86_64 grpcwebproxy && chmod +x grpcwebproxy

FROM scratch

COPY --from=build /grpcwebproxy /
ENTRYPOINT ["/grpcwebproxy"]
