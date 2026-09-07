---
description: Repository guidance for the OPC UA Server ACAP application: C code, D-Bus and OPC UA contracts, manifest configuration, packaging, builds, and validation.
applyTo: "**"
---

# OPC UA Server ACAP

## Scope and architecture

- This is an ACAP version 4 native application. Always write the platform name as `AXIS OS`; use `ACAP` for Axis Camera Application Platform applications and packages.
- It reads temperature-sensor data from `com.axis.TemperatureController` and I/O-port state from `com.axis.IOControl.State` over the system D-Bus, then exposes those values through an `open62541` OPC UA server.
- This is an example and boilerplate application, not a production-ready OPC UA service.
- Keep existing responsibilities separated:
  - `opcua_server.c` owns process startup/shutdown, the GLib main loop, AXParameter setup, D-Bus signal handling, and OPC UA server restarts when the port changes.
  - `opcua_dbus.c` owns system D-Bus proxies, method calls, subscriptions, and `GVariant` signal/result unpacking.
  - `opcua_open62541.c` owns the `UA_Server`, its worker thread, and OPC UA variable-node creation and updates.
  - `opcua_tempsensors.c` and `opcua_portsio.c` own the dynamically-sized sensor/port labels and subscription IDs.
  - `opcua_common.h` provides common GLib, logging, and standard-library dependencies.

## Contract changes

- Keep `manifest.json`, the application executable, and AXParameter group aligned: `appName` and `PROG` are both `opcuaserver`.
- The manifest parameter `port` is an integer in the inclusive range `1024..65535`, defaulting to `4840`. Keep its name, range, default, and `port_callback()` validation synchronized.
- A port change intentionally stops, joins, and recreates the OPC UA server. Preserve the `ua_server_running` lifecycle and do not add access to the global `UA_Server` from uncoordinated threads.
- When adding a D-Bus capability, update all applicable surfaces together: manifest `resources.dbus.requiredMethods`, D-Bus service/object/interface constants and method or signal parsing in `opcua_dbus.c`, initialization or signal handling in `opcua_server.c`, and OPC UA node creation/update in `opcua_open62541.c`.
- Temperature nodes are `Double` values and I/O-port nodes are `Boolean` values. Labels must remain consistent between initial node creation and later updates because they are used as namespace-1 string node IDs.
- Preserve D-Bus ownership rules: unref successful `GVariant` results and proxies, free `GError` values after handling them, and do not use a result value when the call returned `NULL`.

## C conventions

- Build with the project C flags, including `-Wall`, `-Werror`, `-Wformat=2`, and strict prototype checks. Treat warnings as errors.
- Follow the existing C style: 4-space indentation, Allman braces, declarations at the start of a block, `NULL != value` comparisons, braces also around single-line blocks, and explicit error paths.
- New C source and header files use the existing Apache-2.0 Axis copyright and license header.
- Use `LOG_I` and `LOG_E` from `opcua_common.h`, retaining the existing `__FILE__/__FUNCTION__` error-context convention.
- Retain assertions for internal invariants, but use explicit return-value and `GError` handling for external failures.
- Preserve the GLib and open62541 types at their API boundaries, rather than substituting incompatible standard C types.
- Always set `const` on anything that can be `const`.

## Packaging and generated artifacts

- `Dockerfile` cross-compiles both `aarch64` and `armv7hf` packages with the ACAP native SDK, builds static `open62541`, and invokes `acap-build`.
- Keep third-party version and SHA256 changes paired. Renovate manages routine dependency updates.
- Do not hand-edit generated root artifacts: `*.eap`, `*_LICENSE.txt`, `opcuaserver`, object files, or `pa*.conf`.
- Regenerate packages with the container build.

## Build and validation

- Build both package architectures with `make -j "$(nproc)" dockerbuild` or `make -j "$(nproc)" podmanbuild`.
- Use `make aarch64.docker` or `make armv7hf.docker` for a focused Docker build; the matching Podman targets are `aarch64.podman` and `armv7hf.podman`.
- There is no automated test suite. For C, manifest, Dockerfile, or dependency changes, run the relevant container build.
- `LINT.md` documents local Super-Linter commands. Run the appropriate focused linter for formatting, Markdown, JSON, Dockerfile, or workflow changes.
- Validate each changed cross-file contract before finishing, and leave unrelated generated packages and dependency pins untouched.
