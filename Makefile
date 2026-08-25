# Target OS: linux, windows, macos
# If not specified, detect the native OS.
ifeq ($(TARGET_OS),)
    ifeq ($(OS),Windows_NT)
        TARGET_OS = windows
    else
        UNAME_S := $(shell uname -s)

        ifeq ($(UNAME_S),Darwin)
            TARGET_OS = macos
        else ifeq ($(UNAME_S),Linux)
            TARGET_OS = linux
        else
            $(error Unsupported host OS: $(UNAME_S))
        endif
    endif
endif

ifeq ($(filter $(TARGET_OS),linux windows macos),)
    $(error Invalid TARGET_OS '$(TARGET_OS)'. Use linux, windows, or macos)
endif

$(info Target OS: $(TARGET_OS))

CXX = g++
CC  = gcc
ASM = gcc

ifeq ($(TARGET_OS),windows)
    CC  = x86_64-w64-mingw32-gcc
    CXX = x86_64-w64-mingw32-g++
    ASM = x86_64-w64-mingw32-gcc
endif

CXXFLAGS  = -std=c++17 -Wall -Iinclude -Isrc -Llib -O2 -g -MMD -MP
CFLAGS    = -Wall -Iinclude -Isrc -Llib -O2 -g -MMD -MP
ASMFLAGS  = -x assembler -c

SRC_DIR   = src
BUILD_DIR = build/$(TARGET_OS)
LIB_DIR   = deps
SDL2_WIN_PREFIX = $(LIB_DIR)/SDL2/x86_64-w64-mingw32
FREETYPE_WIN_PREFIX = $(LIB_DIR)/freetype/x86_64-w64-mingw32

ifeq ($(TARGET_OS),linux)
    LDFLAGS += -lSDL2 -lGL
endif

ifeq ($(TARGET_OS),windows)
    CXXFLAGS += -I$(SDL2_WIN_PREFIX)/include \
                -I$(FREETYPE_WIN_PREFIX)/include/freetype2
    CFLAGS   += -I$(SDL2_WIN_PREFIX)/include \
                -I$(FREETYPE_WIN_PREFIX)/include/freetype2
    LDFLAGS  += -L$(SDL2_WIN_PREFIX)/lib -L$(FREETYPE_WIN_PREFIX)/lib \
                -lmingw32 -lSDL2main -lSDL2 -lfreetype -lopengl32 -mwindows
endif

ifeq ($(TARGET_OS),macos)
    LDFLAGS += -lSDL2 -framework OpenGL
endif

CXXFLAGS += -I$(LIB_DIR)/gj-image/include \
			-I$(LIB_DIR)/gj-model/include
CFLAGS   += -I$(LIB_DIR)/gj-image/include \
			-I$(LIB_DIR)/gj-model/include

LDFLAGS  += -L$(LIB_DIR)/gj-image/build/$(TARGET_OS) -lgj_image \
			-L$(LIB_DIR)/gj-model/build/$(TARGET_OS) -lgj_model

# FreeType on Linux via host pkg-config; Windows uses FREETYPE_WIN_PREFIX above.
ifeq ($(TARGET_OS),linux)
    CXXFLAGS += $(shell pkg-config --cflags freetype2)
    LDFLAGS  += $(shell pkg-config --libs freetype2)
endif

# Find all source files
SRC_C   = $(shell find $(SRC_DIR) -name "*.c")
SRC_CPP = $(shell find $(SRC_DIR) -name "*.cpp")
SRC_ASM = $(shell find $(SRC_DIR) -name "*.asm")

# Convert to object files
OBJ_C   = $(patsubst $(SRC_DIR)/%.c,   $(BUILD_DIR)/%.o, $(SRC_C))
OBJ_CPP = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_CPP))
OBJ_ASM = $(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, $(SRC_ASM))

OBJ = $(OBJ_C) $(OBJ_CPP) $(OBJ_ASM)

# For tracking header dependencies
DEP_FILES = $(OBJ:.o=.d)

TARGET = gj-engine
DIST_DIR = dist/$(TARGET_OS)

IMAGE_LIB = $(LIB_DIR)/gj-image/build/$(TARGET_OS)/libgj_image.a
MODEL_LIB = $(LIB_DIR)/gj-model/build/$(TARGET_OS)/libgj_model.a

ifeq ($(TARGET_OS),windows)
    TARGET_OUT = $(DIST_DIR)/$(TARGET).exe
    # Runtime DLLs that must sit next to the exe (Wine / Windows).
    MINGW_DLL_GCC    := $(shell $(CXX) -print-file-name=libgcc_s_seh-1.dll)
    MINGW_DLL_STDCPP := $(shell $(CXX) -print-file-name=libstdc++-6.dll)
    MINGW_DLL_PTHREAD := $(shell $(CXX) -print-file-name=libwinpthread-1.dll)
    WIN_RUNTIME_DLLS = \
        $(SDL2_WIN_PREFIX)/bin/SDL2.dll \
        $(FREETYPE_WIN_PREFIX)/bin/libfreetype.dll \
        $(MINGW_DLL_GCC) \
        $(MINGW_DLL_STDCPP) \
        $(MINGW_DLL_PTHREAD)
else
    TARGET_OUT = $(DIST_DIR)/$(TARGET)
endif

# Default target
ifeq ($(TARGET_OS),windows)
all: $(TARGET_OUT) copy-windows-dlls
else
all: $(TARGET_OUT)
endif

# Link (objects first, then libs in LDFLAGS)
$(TARGET_OUT): $(OBJ) $(IMAGE_LIB) $(MODEL_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

ifeq ($(TARGET_OS),windows)
.PHONY: copy-windows-dlls
copy-windows-dlls: $(TARGET_OUT)
	@mkdir -p $(DIST_DIR)
	@for dll in $(WIN_RUNTIME_DLLS); do \
		if [ ! -f "$$dll" ]; then echo "missing runtime DLL: $$dll"; exit 1; fi; \
		cp -f "$$dll" "$(DIST_DIR)/"; \
		echo "copied $$(basename $$dll) -> $(DIST_DIR)/"; \
	done
endif

$(IMAGE_LIB):
	$(MAKE) -C $(LIB_DIR)/gj-image TARGET_OS=$(TARGET_OS)

$(MODEL_LIB):
	$(MAKE) -C $(LIB_DIR)/gj-model TARGET_OS=$(TARGET_OS)

# Compile C
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile ASM
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

# Clean current TARGET_OS artifacts
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) $(TARGET) $(TARGET).exe
	rm -f SDL2.dll libfreetype.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll
	$(MAKE) -C $(LIB_DIR)/gj-image clean
	$(MAKE) -C $(LIB_DIR)/gj-model clean

# Build every platform this host can produce.
# linux: always (when this Makefile runs on Linux, or via TARGET_OS=linux)
# windows: if MinGW cross-compiler is installed
# macos: only when the host is Darwin (no Linux->macOS cross by default)
AVAILABLE_TARGETS :=
ifeq ($(OS),Windows_NT)
    AVAILABLE_TARGETS += windows
else
    UNAME_HOST := $(shell uname -s)
    ifeq ($(UNAME_HOST),Darwin)
        AVAILABLE_TARGETS += macos
    else
        AVAILABLE_TARGETS += linux
    endif
endif

ifneq ($(shell command -v x86_64-w64-mingw32-g++ 2>/dev/null),)
    ifeq ($(filter windows,$(AVAILABLE_TARGETS)),)
        AVAILABLE_TARGETS += windows
    endif
endif

.PHONY: all-targets
all-targets:
	@echo "Building targets:$(AVAILABLE_TARGETS)"
	@for os in $(AVAILABLE_TARGETS); do \
		echo ""; \
		echo "==== $$os ===="; \
		$(MAKE) TARGET_OS=$$os all || exit 1; \
	done
	@echo ""
	@echo "Done. Outputs under dist/"

.PHONY: clean-all
clean-all:
	rm -rf build dist
	rm -f SDL2.dll libfreetype.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll
	$(MAKE) -C $(LIB_DIR)/gj-image clean
	$(MAKE) -C $(LIB_DIR)/gj-model clean

-include $(DEP_FILES)

.PHONY: all deps clean
