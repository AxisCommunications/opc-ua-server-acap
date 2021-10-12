# server-acap

## Build

### On host with ACAP SDK installed

```sh
# With the enviromnent initialized, use:
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
