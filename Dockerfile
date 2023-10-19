# syntax=docker/dockerfile:1.3
FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libhiredis-dev \
    libgrpc++-dev \
    protobuf-compiler-grpc \
    libprotobuf-dev \
    ninja-build \
    curl \
    ccache \
    && rm -rf /var/lib/apt/lists/*

RUN curl -sSL "https://github.com/bufbuild/buf/releases/download/v1.27.0/buf-$(uname -s)-$(uname -m)" -o /usr/bin/buf \
    && chmod +x /usr/bin/buf

COPY . .

ENV CCACHE_DIR /ccache
RUN --mount=type=cache,target=/cache/ cmake -B/build -H. -GNinja \
    -DCMAKE_INSTALL_PREFIX=/build/install \
    -DCMAKE_BUILD_TYPE=Release -DPREFER_STATIC=ON \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    && cmake --build /build --target install


FROM debian:bookworm-slim AS runner

COPY --from=build /usr/share/grpc/roots.pem /usr/share/grpc/roots.pem
COPY --from=build /build/install/. /usr/

ENTRYPOINT ["axy"]
