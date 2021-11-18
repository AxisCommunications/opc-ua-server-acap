# Copyright (C) 2021 Axis Communications AB, Lund, Sweden
# SPDX-License-Identifier: Apache-2.0

.PHONY: %.eap dockerbuild 3rd-party-clean clean very-clean

PROG = opcuaserver
OBJS = opcua_server.o opcua_dbus.o opcua_open62541.o opcua_tempsensors.o
STRIP ?= strip

PKGS =  gio-2.0 glib-2.0
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

TAG := server-acap

# open62541
OPEN62541_VERSION = 1.2
OPEN62541 = open62541-$(OPEN62541_VERSION)
OPEN62541_BUILD = $(OPEN62541)/build

LIBOPEN62541 = $(OPEN62541_BUILD)/bin/libopen62541.a
CFLAGS += -I $(OPEN62541)/include -I $(OPEN62541_BUILD)/src_generated -I $(OPEN62541)/arch -I $(OPEN62541)/deps -I $(OPEN62541)/plugins/include
LDLIBS += $(OPEN62541_BUILD)/bin/libopen62541.a

CFLAGS += -Wformat=2 -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Wdisabled-optimization -Wall -Werror

# main targets
all: $(PROG)
	$(STRIP) $(PROG)

$(OBJS): $(LIBOPEN62541)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) $(LDLIBS) -o $@

# open62541 targets
$(OPEN62541):
	curl -L https://github.com/open62541/open62541/archive/refs/tags/v$(OPEN62541_VERSION).tar.gz | tar xz

$(OPEN62541_BUILD): $(OPEN62541)
	mkdir -p $(OPEN62541_BUILD)

$(OPEN62541_BUILD)/Makefile: | $(OPEN62541_BUILD)
	cd $(OPEN62541_BUILD) && \
	cmake -j ..

$(LIBOPEN62541): $(OPEN62541_BUILD)/Makefile
	make -j -C $(OPEN62541_BUILD)

# docker build container targets
%.eap:
	DOCKER_BUILDKIT=1 docker build --build-arg ARCH=$(@:.eap=) -o type=local,dest=. .

dockerbuild: armv7hf.eap aarch64.eap

# clean targets
3rd-party-clean:
	rm -rf $(OPEN62541_BUILD)

clean:
ifneq (,$(wildcard ./package.conf.orig))
	mv package.conf.orig package.conf
endif
	rm -f $(PROG) *.o *.eap *LICENSE.txt

very-clean: clean 3rd-party-clean
	rm -rf *.eap *.eap.old $(OPEN62541) eap
