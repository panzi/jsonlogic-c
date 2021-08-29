CC=gcc
CFLAGS=-std=c11 -Wall -Werror -pedantic
BUILD_DIR=build
LIB_OBJS=$(BUILD_DIR)/obj/array.o \
         $(BUILD_DIR)/obj/boolean.o \
         $(BUILD_DIR)/obj/extras.o \
         $(BUILD_DIR)/obj/compare.o \
         $(BUILD_DIR)/obj/json.o \
         $(BUILD_DIR)/obj/jsonlogic.o \
         $(BUILD_DIR)/obj/number.o \
         $(BUILD_DIR)/obj/object.o \
         $(BUILD_DIR)/obj/string.o
LIBS=-lm

SO_OBJS=$(patsubst $(BUILD_DIR)/obj/%,$(BUILD_DIR)/shared-obj/%,$(LIB_OBJS))

AR=ar
SO_EXT=.so
SO_PREFIX=lib
BIN_EXT=
TARGET=$(shell uname|tr '[:upper:]' '[:lower:]')$(shell getconf LONG_BIT)
RELEASE=OFF
PREFIX=/usr/local
SO_FLAGS=-fPIC
SHARED_BIN_OBJS=

ifneq ($(patsubst %64,64,$(TARGET)),64)
    $(error only 64 bit systems are supported)
endif

ifeq ($(patsubst %32,32,$(TARGET)),32)
    CFLAGS += -m32
else
ifeq ($(patsubst %64,64,$(TARGET)),64)
    CFLAGS += -m64
endif
endif

ifeq ($(patsubst win%,win,$(TARGET)),win)
    CFLAGS   += -Wno-pedantic-ms-format -DWIN_EXPORT
    BIN_EXT   = .exe
    SO_EXT    = .dll
    SO_PREFIX =
else
ifeq ($(patsubst darwin%,darwin,$(TARGET)),darwin)
    CC      = clang
    CFLAGS += -Qunused-arguments
    SO_EXT  = .dylib
endif
endif

ifeq ($(TARGET),win32)
    CC=i686-w64-mingw32-gcc
else
ifeq ($(TARGET),win64)
    CC=x86_64-w64-mingw32-gcc
endif
endif

BUILD_DIR := $(BUILD_DIR)/$(TARGET)

LIB_DIRS=-L$(BUILD_DIR)/lib
INC_DIRS=-Isrc
EXAMPLES=$(BUILD_DIR)/examples/parse_json$(BIN_EXT)
EXAMPLES_SHARED=$(BUILD_DIR)/examples-shared/parse_json$(BIN_EXT)
LIB=$(BUILD_DIR)/lib/libjsonlogic.a
SO=$(BUILD_DIR)/lib/$(SO_PREFIX)jsonlogic$(SO_EXT)
INC=$(BUILD_DIR)/include/jsonlogic.h

ifeq ($(patsubst darwin%,darwin,$(TARGET)),darwin)
    PSEUDO_STATIC=ON
else
    PSEUDO_STATIC=OFF
endif

ifeq ($(PSEUDO_STATIC),ON)
    STATIC_LIBS = $(LIB) $(LIBS)
    STATIC_FLAG =
else
ifeq ($(PSEUDO_STATIC),OFF)
    STATIC_LIBS = $(LIB_DIRS) -ljsonlogic $(LIBS)
    STATIC_FLAG = -static
else
    $(error illegal value for PSEUDO_STATIC=$(PSEUDO_STATIC))
endif
endif

ifeq ($(RELEASE),ON)
    CFLAGS    += -DNDEBUG
    BUILD_DIR := $(BUILD_DIR)/release
else
ifeq ($(RELEASE),OFF)
    CFLAGS    += -g
    BUILD_DIR := $(BUILD_DIR)/debug
else
    $(error illegal value for RELEASE=$(RELEASE))
endif
endif

.PHONY: static shared lib so inc examples examples_shared clean install uninstall

static: lib inc

shared: so inc

lib: $(LIB)

examples: $(EXAMPLES)

examples_shared: $(EXAMPLES_SHARED)

so: $(SO)

inc: $(inc)

install: $(LIB) $(SO) $(INC) $(BIN)
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp $(LIB) $(SO) $(PREFIX)/lib
	cp $(INC) $(PREFIX)/include

uninstall:
	rm $(PREFIX)/lib/libjsonlogic.a \
	   $(PREFIX)/lib/$(SO_PREFIX)jsonlogic$(SO_EXT) \
	   $(PREFIX)/include/jsonlogic_defs.h \
	   $(PREFIX)/include/jsonlogic_extras.h \
	   $(PREFIX)/include/jsonlogic.h

$(BUILD_DIR)/examples/%$(BIN_EXT): $(BUILD_DIR)/obj/examples/%.o $(LIB)
	@mkdir -p $(BUILD_DIR)/examples
	$(CC) $(CFLAGS) $(STATIC_FLAG) $< $(STATIC_LIBS) -o $@

$(BUILD_DIR)/examples-shared/%$(BIN_EXT): $(BUILD_DIR)/shared-obj/examples/%.o $(SO)
	@mkdir -p $(BUILD_DIR)/examples-shared
	$(CC) $(CFLAGS) $< $(LIB_DIRS) -ljsonlogic $(LIBS) -o $@

$(BUILD_DIR)/obj/%.o: src/%.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/obj
	$(CC) $(CFLAGS) $(INC_DIRS) $< -c -o $@

$(BUILD_DIR)/shared-obj/%.o: src/%.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/shared-obj
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) $< -c -o $@

$(BUILD_DIR)/obj/json.o: src/json.c src/stringify.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/obj
	$(CC) $(CFLAGS) $(INC_DIRS) $< -c -o $@

$(BUILD_DIR)/shared-obj/json.o: src/json.c src/stringify.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/shared-obj
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) $< -c -o $@

$(BUILD_DIR)/obj/examples/%.o: examples/%.c
	@mkdir -p $(BUILD_DIR)/obj/examples
	$(CC) $(CFLAGS) $(INC_DIRS) $< -c -o $@

$(BUILD_DIR)/shared-obj/examples/%.o: examples/%.c
	@mkdir -p $(BUILD_DIR)/shared-obj/examples
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) $< -c -o $@

$(BUILD_DIR)/examples/%$(BIN_EXT): $(BUILD_DIR)/obj/examples/%.o
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

$(BUILD_DIR)/examples-shared/%$(BIN_EXT): $(BUILD_DIR)/shared-obj/examples/%.o
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

$(LIB): $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)/lib
	@rm $@ 2>/dev/null || true
	$(AR) rcs $@ $^

$(SO): $(SO_OBJS)
	@mkdir -p $(BUILD_DIR)/lib
	$(CC) $(CFLAGS) -shared $^ $(LIB_DIRS) $(LIBS) -o $@

$(INC): src/jsonlogic.h src/jsonlogic_defs.h src/jsonlogic_extras.h
	@mkdir -p $(BUILD_DIR)/include
	cp src/jsonlogic.h $(BUILD_DIR)/include/jsonlogic.h
	cp src/jsonlogic_defs.h $(BUILD_DIR)/include/jsonlogic_defs.h
	cp src/jsonlogic_extras.h $(BUILD_DIR)/include/jsonlogic_extras.h

clean:
	rm -vf $(LIB_OBJS) $(SO_OBJS) $(LIB) $(EXAMPLES) $(EXAMPLES_SHARED) || true
