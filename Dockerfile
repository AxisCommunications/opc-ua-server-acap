ARG ARCH=aarch64
ARG SDK_VERSION=12.11.0
ARG SDK_IMAGE=docker.io/axisecp/acap-native-sdk
ARG BUILD_DIR=/usr/local/src
ARG ACAP_BUILD_DIR="$BUILD_DIR"/server-acap
ARG OPEN62541_VERSION=1.5.8
ARG OPEN62541_SHA256=cf7951baf253c0537b3397e4ce3ff13930542abcb6ffc3b9cb082af88f95c300

FROM $SDK_IMAGE:$SDK_VERSION-$ARCH AS builder
ARG BUILD_DIR
ARG ACAP_BUILD_DIR
ARG OPEN62541_VERSION
ARG OPEN62541_SHA256
ENV DEBIAN_FRONTEND=noninteractive

# Install additional build dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    cmake

# open62541
ARG OPEN62541_DIR="$BUILD_DIR"/open62541
ARG OPEN62541_SRC_DIR="$OPEN62541_DIR"/open62541-$OPEN62541_VERSION
ARG OPEN62541_BUILD_DIR="$OPEN62541_DIR"/build

WORKDIR "$OPEN62541_DIR"
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN curl -L -o open62541.tar.gz https://github.com/open62541/open62541/archive/refs/tags/v$OPEN62541_VERSION.tar.gz && \
    echo "$OPEN62541_SHA256  open62541.tar.gz" | sha256sum -c - && \
    tar xzf open62541.tar.gz && \
    rm open62541.tar.gz
WORKDIR "$OPEN62541_BUILD_DIR"
RUN . /opt/axis/acapsdk/environment-setup* && \
    cmake \
    -DCMAKE_INSTALL_PREFIX="$SDKTARGETSYSROOT"/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_BUILD_EXAMPLES=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DUA_ENABLE_NODEMANAGEMENT=ON \
    -DUA_MULTITHREADING=100 \
    "$OPEN62541_SRC_DIR"
RUN make -j "$(nproc)" install

# ACAP application
WORKDIR "$ACAP_BUILD_DIR"
COPY LICENSE \
     Makefile \
     *.c \
     *.h \
     manifest.json \
     ./
RUN . /opt/axis/acapsdk/environment-setup* && \
    acap-build .

FROM scratch
ARG ACAP_BUILD_DIR
COPY --from=builder "$ACAP_BUILD_DIR"/*eap "$ACAP_BUILD_DIR"/*LICENSE.txt /
