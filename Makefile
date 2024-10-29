.PHONY: %.eap dockerbuild clean

PROG = opcuaserver
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
STRIP ?= strip

PKGS =  gio-2.0 glib-2.0 axparameter open62541
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

CFLAGS += -Wformat=2 -Wpointer-arith -Wbad-function-cast -Wstrict-prototypes -Wdisabled-optimization -Wall -Werror
LDFLAGS += -flto=auto

# main targets
all: $(PROG)
	$(STRIP) $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) $(LDLIBS) -o $@

# docker build container targets
%.eap:
	DOCKER_BUILDKIT=1 docker build --build-arg ARCH=$(*F) -o type=local,dest=. "$(CURDIR)"

dockerbuild: armv7hf.eap aarch64.eap

# clean targets
clean:
	rm -f $(PROG) *.o *.eap* *LICENSE.txt pa*conf*
