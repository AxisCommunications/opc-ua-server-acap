# OPC UA Server ACAP

This directory contains the source code to build a small ACAP application that
uses D-Bus to get sensor data from `com.axis.TemperatureController` and expose
it as [OPC UA](https://en.wikipedia.org/wiki/OPC_Unified_Architecture) with an
[open62541](https://open62541.org/) server. It serves as an example of how easy
it actually is to integrate any Axis device in an OPC UA system.

## Build

### On host with ACAP SDK installed

```sh
# With the environment initialized, use:
create-package.sh
```

### Using ACAP SDK build container and Docker

The handling of this is integrated in the [Makefile](Makefile), so if you have
Docker on your computer all you need to do is:

```sh
make dockerbuild
```

or perhaps

```sh
make -j dockerbuild
```
