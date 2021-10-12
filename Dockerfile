ARG ARCH=armv7hf
ARG ACAP_SDK_VERSION=3.4
ARG SDK_IMAGE=axisecp/acap-sdk
ARG BUILD_DIR=/usr/local/src/server-acap

FROM $SDK_IMAGE:$ACAP_SDK_VERSION-armv7hf-ubuntu20.04 AS armv7hf

FROM $SDK_IMAGE:$ACAP_SDK_VERSION-aarch64-ubuntu20.04 AS aarch64

FROM $ARCH AS builder
ARG BUILD_DIR
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    cmake
WORKDIR "$BUILD_DIR"
COPY LICENSE \
     Makefile \
     *.c \
     *.h \
     *.conf \
     ./
RUN . /opt/axis/acapsdk/environment-setup* && \
    create-package.sh

FROM busybox:1.34.0
ARG BUILD_DIR
WORKDIR /eap
COPY --from=builder "$BUILD_DIR"/*eap ./
COPY --from=builder "$BUILD_DIR"/*LICENSE.txt ./
