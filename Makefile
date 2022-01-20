CC=gcc
CFLAGS=-std=c11 -Wall -Werror -pedantic -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64
BUILD_DIR=build
LIB_OBJS=$(BUILD_DIR)/obj/tbl/builtins_tbl.o \
         $(BUILD_DIR)/obj/tbl/extras_tbl.o \
         $(BUILD_DIR)/obj/tbl/certlogic_tbl.o \
         $(BUILD_DIR)/obj/tbl/certlogic_extras_tbl.o \
         $(BUILD_DIR)/obj/array.o \
         $(BUILD_DIR)/obj/boolean.o \
         $(BUILD_DIR)/obj/extras.o \
         $(BUILD_DIR)/obj/hash.o \
         $(BUILD_DIR)/obj/iterator.o \
         $(BUILD_DIR)/obj/compare.o \
         $(BUILD_DIR)/obj/error.o \
         $(BUILD_DIR)/obj/json.o \
         $(BUILD_DIR)/obj/jsonlogic.o \
         $(BUILD_DIR)/obj/certlogic.o \
         $(BUILD_DIR)/obj/number.o \
         $(BUILD_DIR)/obj/object.o \
         $(BUILD_DIR)/obj/operations.o \
         $(BUILD_DIR)/obj/string.o
LIBS=-lm

SO_OBJS=$(patsubst $(BUILD_DIR)/obj/%,$(BUILD_DIR)/shared-obj/%,$(LIB_OBJS))

AR=ar
SO_EXT=.so
SO_PREFIX=lib
BIN_EXT=
TARGET=$(shell uname -s|tr '[:upper:]' '[:lower:]')-$(shell uname -m)
RELEASE=OFF
PREFIX=/usr/local
SO_FLAGS=-fPIC
SHARED_BIN_OBJS=
NDK_VERSION=r23b
ANDROID_VERSION=31
NDK_PATH=ndk/android-ndk-$(NDK_VERSION)

ifeq ($(patsubst %-i686,i686,$(TARGET)),i686)
    CFLAGS += -m32 -march=i686
else
ifeq ($(patsubst %-x86_64,x86_64,$(TARGET)),x86_64)
    CFLAGS += -m64 -march=x86-64
else
ifneq ($(patsubst android-%,android,$(TARGET)),android)
    CFLAGS += -march=$(shell echo $(TARGET)|sed 's/.*-//')
endif
endif
endif

ifeq ($(patsubst mingw-%,mingw,$(TARGET)),mingw)
    CFLAGS   += -Wno-pedantic-ms-format -static-libgcc
    BIN_EXT   = .exe
    SO_PREFIX =
    SO_EXT    = .dll
else
ifeq ($(patsubst darwin%,darwin,$(TARGET)),darwin)
    CC      = clang
    CFLAGS += -Qunused-arguments
    SO_EXT  = .dylib
else
ifeq ($(patsubst android-%,android,$(TARGET)),android)
    ARCH    = $(shell echo $(TARGET)|sed 's/^android-//')
    CC      = $(NDK_PATH)/toolchains/llvm/prebuilt/linux-x86_64/bin/$(ARCH)-linux-android$(ANDROID_VERSION)-clang
    CFLAGS += -Qunused-arguments
else
ifneq ($(shell echo $(TARGET)|sed 's/-.*//'),$(shell uname -s|tr '[:upper:]' '[:lower:]'))
    $(error platform of target $(TARGET) is not supported)
endif
endif
endif
endif

ifeq ($(TARGET),mingw-i686)
    CC=i686-w64-mingw32-gcc
else
ifeq ($(TARGET),mingw-x86_64)
    CC=x86_64-w64-mingw32-gcc
endif
endif

BUILD_DIR := $(BUILD_DIR)/$(TARGET)

LIB_DIRS=-L$(BUILD_DIR)/lib
INC_DIRS=-Isrc
EXAMPLES=$(BUILD_DIR)/examples/benchmark$(BIN_EXT) \
         $(BUILD_DIR)/examples/parse_json$(BIN_EXT) \
         $(BUILD_DIR)/examples/jsonlogic$(BIN_EXT) \
         $(BUILD_DIR)/examples/jsonlogic_extras$(BIN_EXT) \
         $(BUILD_DIR)/examples/certlogic$(BIN_EXT) \
         $(BUILD_DIR)/examples/certlogic_extras$(BIN_EXT)
EXAMPLES_SHARED=$(patsubst $(BUILD_DIR)/examples/%,$(BUILD_DIR)/examples-shared/%,$(EXAMPLES))
LIB=$(BUILD_DIR)/lib/libjsonlogic_static.a
SO=$(BUILD_DIR)/lib/$(SO_PREFIX)jsonlogic$(SO_EXT)
INC=$(BUILD_DIR)/include/jsonlogic.h \
    $(BUILD_DIR)/include/jsonlogic_defs.h \
    $(BUILD_DIR)/include/jsonlogic_extras.h
OPS_SRC_DIR=$(BUILD_DIR)/src

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
    STATIC_LIBS = $(LIB_DIRS) -ljsonlogic_static $(LIBS)
    STATIC_FLAG = -static
else
    $(error illegal value for PSEUDO_STATIC=$(PSEUDO_STATIC))
endif
endif

ifeq ($(RELEASE),ON)
    CFLAGS    += -DNDEBUG -O3
    BUILD_DIR := $(BUILD_DIR)/release
else
ifeq ($(RELEASE),OFF)
    CFLAGS    += -g -ftrapv -O0
    BUILD_DIR := $(BUILD_DIR)/debug
else
    $(error illegal value for RELEASE=$(RELEASE))
endif
endif

.PHONY: static shared lib so inc examples examples_shared \
        clean install uninstall test test_shared valgrind \
        ops install_static install_shared

static: lib inc

shared: so inc

test: $(BUILD_DIR)/bin/test$(BIN_EXT)
	$<

test_shared: $(BUILD_DIR)/bin/test_shared$(BIN_EXT)
	LD_LIBRARY_PATH=$(BUILD_DIR)/lib $<

valgrind: $(BUILD_DIR)/bin/test_shared$(BIN_EXT)
	LD_LIBRARY_PATH=$(BUILD_DIR)/lib valgrind --tool=memcheck --leak-check=full --track-origins=yes $<

ops: $(BUILD_DIR)/bin/compile_operations$(BIN_EXT)
	@mkdir -p $(OPS_SRC_DIR)
	$< $(OPS_SRC_DIR)

lib: $(LIB)

examples: $(EXAMPLES)

examples_shared: $(EXAMPLES_SHARED)

so: $(SO)

inc: $(INC)

install: $(LIB) $(SO) $(INC)
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp $(LIB) $(SO) $(PREFIX)/lib
	cp $(INC) $(PREFIX)/include

install_static: $(LIB) $(INC)
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp $(LIB) $(PREFIX)/lib
	cp $(INC) $(PREFIX)/include

install_shared: $(SO) $(INC)
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp $(SO) $(PREFIX)/lib
	cp $(INC) $(PREFIX)/include

uninstall:
	rm $(PREFIX)/lib/libjsonlogic.a \
	   $(PREFIX)/lib/$(SO_PREFIX)jsonlogic$(SO_EXT) \
	   $(PREFIX)/include/jsonlogic_defs.h \
	   $(PREFIX)/include/jsonlogic_extras.h \
	   $(PREFIX)/include/jsonlogic.h

$(BUILD_DIR)/bin/compile_operations$(BIN_EXT): $(BUILD_DIR)/obj/compile_operations.o $(BUILD_DIR)/obj/hash.o src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/bin
	$(CC) $(CFLAGS) $(STATIC_FLAG) $< $(BUILD_DIR)/obj/hash.o $(LIBS) -o $@

$(BUILD_DIR)/bin/test$(BIN_EXT): $(BUILD_DIR)/obj/test.o src/jsonlogic.h src/jsonlogic_intern.h $(LIB)
	@mkdir -p $(BUILD_DIR)/bin
	$(CC) $(CFLAGS) $(STATIC_FLAG) $< $(STATIC_LIBS) -o $@

# shared binary but statically linked objects, because we're using internal APIs in tests
$(BUILD_DIR)/bin/test_shared$(BIN_EXT): $(BUILD_DIR)/shared-obj/test.o src/jsonlogic.h src/jsonlogic_intern.h $(SO_OBJS) $(SO)
	@mkdir -p $(BUILD_DIR)/bin
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) $(SO_OBJS) $(LIBS) $< -o $@

$(BUILD_DIR)/examples/%$(BIN_EXT): $(BUILD_DIR)/obj/examples/%.o $(LIB)
	@mkdir -p $(BUILD_DIR)/examples
	$(CC) $(CFLAGS) $(STATIC_FLAG) $< $(STATIC_LIBS) -o $@

$(BUILD_DIR)/examples-shared/%$(BIN_EXT): $(BUILD_DIR)/shared-obj/examples/%.o $(SO)
	@mkdir -p $(BUILD_DIR)/examples-shared
	$(CC) $(CFLAGS) $(SO_FLAGS) $< $(LIB_DIRS) -ljsonlogic $(LIBS) -o $@

# compile and run compile_operations natively
#$(BUILD_DIR)/src/builtins_tbl.c: $(BUILD_DIR)/bin/compile_operations$(BIN_EXT)
$(BUILD_DIR)/src/builtins_tbl.c: src/jsonlogic_defs.h src/jsonlogic_intern.h src/compile_operations.c
	$(MAKE) OPS_SRC_DIR=$(BUILD_DIR)/src TARGET=$(shell uname -s|tr '[:upper:]' '[:lower:]')-$(shell uname -m) ops
#	@mkdir -p $(BUILD_DIR)/src
#	$< $(BUILD_DIR)/src

$(BUILD_DIR)/src/extras_tbl.c: $(BUILD_DIR)/src/builtins_tbl.c

$(BUILD_DIR)/src/certlogic_tbl.c: $(BUILD_DIR)/src/builtins_tbl.c

$(BUILD_DIR)/src/certlogic_extras_tbl.c: $(BUILD_DIR)/src/builtins_tbl.c

$(BUILD_DIR)/obj/tbl/%.o: $(BUILD_DIR)/src/%.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/obj/tbl
	$(CC) $(CFLAGS) $(INC_DIRS) -DJSONLOGIC_STATIC -DJSONLOGIC_WIN_EXPORT $< -c -o $@

$(BUILD_DIR)/obj/%.o: src/%.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/obj
	$(CC) $(CFLAGS) $(INC_DIRS) -DJSONLOGIC_STATIC -DJSONLOGIC_WIN_EXPORT $< -c -o $@

$(BUILD_DIR)/shared-obj/tbl/%.o: $(BUILD_DIR)/src/%.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/shared-obj/tbl
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) -DJSONLOGIC_WIN_EXPORT $< -c -o $@

$(BUILD_DIR)/shared-obj/%.o: src/%.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/shared-obj
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) -DJSONLOGIC_WIN_EXPORT $< -c -o $@

$(BUILD_DIR)/obj/test.o: tests/test.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/obj
	$(CC) $(CFLAGS) $(INC_DIRS) -DJSONLOGIC_STATIC $< -c -o $@

$(BUILD_DIR)/shared-obj/test.o: tests/test.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/shared-obj
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) -DJSONLOGIC_WIN_EXPORT $< -c -o $@

$(BUILD_DIR)/obj/json.o: src/json.c src/stringify.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/obj
	$(CC) $(CFLAGS) $(INC_DIRS) -DJSONLOGIC_STATIC -DJSONLOGIC_WIN_EXPORT $< -c -o $@

$(BUILD_DIR)/shared-obj/json.o: src/json.c src/stringify.c src/jsonlogic.h src/jsonlogic_intern.h
	@mkdir -p $(BUILD_DIR)/shared-obj
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) -DJSONLOGIC_WIN_EXPORT $< -c -o $@

$(BUILD_DIR)/obj/examples/%.o: examples/%.c
	@mkdir -p $(BUILD_DIR)/obj/examples
	$(CC) $(CFLAGS) $(INC_DIRS) -DJSONLOGIC_STATIC $< -c -o $@

$(BUILD_DIR)/shared-obj/examples/%.o: examples/%.c
	@mkdir -p $(BUILD_DIR)/shared-obj/examples
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) $< -c -o $@

$(BUILD_DIR)/examples/%$(BIN_EXT): $(BUILD_DIR)/obj/examples/%.o
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

$(BUILD_DIR)/examples-shared/%$(BIN_EXT): $(BUILD_DIR)/shared-obj/examples/%.o
	$(CC) $(CFLAGS) $(SO_FLAGS) $(LIBS) $^ -o $@

$(LIB): $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)/lib
	@rm $@ 2>/dev/null || true
	$(AR) rcs $@ $^

$(SO): $(SO_OBJS)
	@mkdir -p $(BUILD_DIR)/lib
	$(CC) $(CFLAGS) -shared $^ $(LIB_DIRS) $(LIBS) -o $@

$(BUILD_DIR)/include/%.h: src/%.h
	@mkdir -p $(BUILD_DIR)/include
	cp $< $@

clean:
	rm -vf $(LIB_OBJS) $(SO_OBJS) $(LIB) $(EXAMPLES) $(EXAMPLES_SHARED) $(INC) \
	       $(BUILD_DIR)/bin/test$(BIN_EXT) $(BUILD_DIR)/bin/test_shared$(BIN_EXT) \
	       $(BUILD_DIR)/obj/test.o $(BUILD_DIR)/shared-obj/test.o \
	       $(BUILD_DIR)/obj/compile_operations.o $(BUILD_DIR)/bin/compile_operations$(BIN_EXT) \
	       $(BUILD_DIR)/src/extras_tbl.c $(BUILD_DIR)/src/builtins_tbl.c \
	       $(BUILD_DIR)/src/certlogic_extras_tbl.c $(BUILD_DIR)/src/certlogic_tbl.c || true
