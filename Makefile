CXX = g++
CC  = gcc
ASM = gcc

CXXFLAGS  = -std=c++17 -Wall -Iinclude -Isrc -Llib -O2 -g -MMD -MP
CFLAGS    = -Wall -Iinclude -Isrc -Llib -O2 -g -MMD -MP
ASMFLAGS  = -x assembler -c

LDFLAGS   = -lSDL2 -lGL

SRC_DIR   = src
BUILD_DIR = build
LIB_DIR   = deps


CXXFLAGS += -I$(LIB_DIR)/gj-image/include \
			-I$(LIB_DIR)/gj-model/include
CFLAGS   += -I$(LIB_DIR)/gj-image/include \
			-I$(LIB_DIR)/gj-model/include

LDFLAGS  += -L$(LIB_DIR)/gj-image/build -lgj_image \
			-L$(LIB_DIR)/gj-model/build -lgj_model

# FreeType flags (for fonts)
CXXFLAGS += $(shell pkg-config --cflags freetype2)
LDFLAGS  += $(shell pkg-config --libs freetype2)

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

TARGET = main

IMAGE_LIB = $(LIB_DIR)/gj-image/build/libgj_image.a
MODEL_LIB = $(LIB_DIR)/gj-model/build/libgj_model.a

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJ) $(IMAGE_LIB) $(MODEL_LIB)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

$(IMAGE_LIB):
	$(MAKE) -C $(LIB_DIR)/gj-image

$(MODEL_LIB):
	$(MAKE) -C $(LIB_DIR)/gj-model

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

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	$(MAKE) -C $(LIB_DIR)/gj-image clean
	$(MAKE) -C $(LIB_DIR)/gj-model clean

-include $(DEP_FILES)

.PHONY: all deps image model clean
