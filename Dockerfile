ARG ARCH=armv7hf
ARG ACAP_SDK_VERSION=3.5
ARG SDK_IMAGE=axisecp/acap-sdk
ARG BUILD_DIR=/usr/local/src/server-acap

FROM $SDK_IMAGE:$ACAP_SDK_VERSION-$ARCH AS builder
ARG BUILD_DIR
# hadolint ignore=DL3008
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    cmake
WORKDIR "$BUILD_DIR"
COPY LICENSE \
     Makefile \
     *.c \
     *.h \
     manifest.json \
     ./
RUN . /opt/axis/acapsdk/environment-setup* && \
    acap-build .

FROM scratch
ARG BUILD_DIR
COPY --from=builder "$BUILD_DIR"/*eap "$BUILD_DIR"/*LICENSE.txt /
