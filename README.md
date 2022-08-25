*Copyright (C) 2022, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# OPC UA Server ACAP

[![Build ACAPs](https://github.com/AxisCommunications/opc-ua-server-acap/actions/workflows/build.yml/badge.svg)](https://github.com/AxisCommunications/opc-ua-server-acap/actions/workflows/build.yml)
[![GitHub Super-Linter](https://github.com/AxisCommunications/opc-ua-server-acap/actions/workflows/super-linter.yml/badge.svg)](https://github.com/AxisCommunications/opc-ua-server-acap/actions/workflows/super-linter.yml)

This directory contains the source code to build a small ACAP application that
uses D-Bus to get

- temperature sensor data from `com.axis.TemperatureController`
- IO port states from `com.axis.IOControl.State`

and expose them as [OPC UA](https://en.wikipedia.org/wiki/OPC_Unified_Architecture) with an
[open62541](https://open62541.org/) server. It serves as an example of how easy
it actually is to integrate any Axis device in an OPC UA system.

## Build

### On host with ACAP SDK installed

```sh
# With the environment initialized, use:
acap-build .
```

### Using ACAP SDK build container and Docker

The handling of this is integrated in the [Makefile](Makefile), so if you have
Docker and `make` on your computer all you need to do is:

```sh
make dockerbuild
```

or perhaps build in parallel:

```sh
make -j dockerbuild
```

If you do have Docker but no `make` on your system:

```sh
# 32-bit ARM, e.g. ARTPEC-6- and ARTPEC-7-based devices
DOCKER_BUILDKIT=1 docker build --build-arg ARCH=armv7hf -o type=local,dest=. .
# 64-bit ARM, e.g. ARTPEC-8-based devices
DOCKER_BUILDKIT=1 docker build --build-arg ARCH=aarch64 -o type=local,dest=. .
```

## License

[Apache 2.0](LICENSE)
