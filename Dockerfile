FROM ubuntu:22.04 AS builder 

ARG GRPC_VERSION="v1.44.0"

RUN apt-get update && apt-get install -y \
    build-essential \
    autoconf \
    libtool \
    pkg-config \
    cmake \
    git \
    libssl-dev \
    qt6-base-dev \
    qt6-tools-dev \
    protobuf-compiler \
    libprotobuf-dev \
    libsodium-dev \
    uuid-dev \
    libgl1-mesa-dev \
    libxkbcommon-x11-dev \
    libxcb-util-dev \
    libxcb-icccm4-dev \
    libxcb-keysyms1-dev \
    libxcb-image0-dev \
    && rm -rf /var/lib/apt/lists/*


RUN git clone --depth 1 -b ${GRPC_VERSION} --recurse-submodules https://github.com/grpc/grpc /opt/grpc-src && \
    cd /opt/grpc-src && \
    mkdir -p cmake/build && \
    cd cmake/build && \
    cmake -DgRPC_INSTALL=ON \
            -DCMAKE_BUILD_TYPE=Release \
            -DgRPC_BUILD_TESTS=OFF \
            -DgRPC_SSL_PROVIDER=package \
            -DgRPC_ZLIB_PROVIDER=package \
            ../.. && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    rm -rf /opt/grpc-src


WORKDIR /build

COPY CMakeLists.txt .

COPY client/CMakeLists.txt client/
COPY server/CMakeLists.txt server/

COPY client/src client/src
COPY server/src server/src
COPY proto/ proto

RUN mkdir build && \
    cd build && \
    cmake .. && \
    cmake --build . -j$(nproc)

FROM ubuntu:22.04

WORKDIR /app

COPY --from=builder /build /app

EXPOSE 50051
