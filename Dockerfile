ARG ARCH=aarch64
ARG SDK_VERSION=12.7.0
ARG SDK_IMAGE=docker.io/axisecp/acap-native-sdk
ARG BUILD_DIR=/usr/local/src
ARG ACAP_BUILD_DIR="$BUILD_DIR"/server-acap
ARG OPEN62541_VERSION=1.4.4
ARG OPEN62541_SHA256=8d92d4d7b293612efcd87bfe3b833fc2a953d83e4d58045a9186b6cacaad4c58

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
