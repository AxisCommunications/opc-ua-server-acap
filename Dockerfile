ARG ARCH=aarch64
ARG ACAP_SDK_VERSION=1.15
ARG SDK_IMAGE=axisecp/acap-native-sdk
ARG BUILD_DIR=/usr/local/src
ARG ACAP_BUILD_DIR="$BUILD_DIR"/server-acap
ARG OPEN62541_VERSION=1.4.4

FROM $SDK_IMAGE:$ACAP_SDK_VERSION-$ARCH AS builder
ARG BUILD_DIR
ARG ACAP_BUILD_DIR
ARG OPEN62541_VERSION
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
RUN curl -L https://github.com/open62541/open62541/archive/refs/tags/v$OPEN62541_VERSION.tar.gz | tar xz
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
