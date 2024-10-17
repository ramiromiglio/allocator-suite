BUILD_DIR := build
SOURCE_DIR := src
SOURCES := $(wildcard $(SOURCE_DIR)/*.cpp)
OBJECTS := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SOURCES)))
DEPENDS := $(patsubst %.cpp, $(BUILD_DIR)/%.d, $(notdir $(SOURCES)))
BIN := bin/main.exe
#CXX := C:/cygwin64/bin/g++.exe
#CXX := C:/MinGW/bin/g++.exe
#CXX := C:/cygwin64/bin/x86_64-w64-mingw32-g++.exe
#INCLUDE_CXX_FLAGS := -Iinclude -IC:/cygwin64/usr/include
CXX := g++

CXX_FLAGS := \
	-std=gnu++17 -Iinclude -Wall -Wextra -O3 -w -Wl,-subsystem,console \

LNK_FLAGS := -Llib

colorize = \e[0;32m$(1)\e[m

all: $(BIN)

-include $(DEPENDS)

$(BIN): $(OBJECTS)
	@$(CXX) $^ $(LNK_FLAGS) -o $(BIN)
	@echo -e "\nBinary constructed as $(call colorize, $(notdir $@))"

$(OBJECTS): $(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@echo -e "Compiling $(call colorize, $(notdir $<)).."
	@$(CXX) $(CXX_FLAGS) -MMD -MP -c -o $@ $<

.PHONY: clean

clean:
	@rm -f $(BIN) $(OBJECTS) $(DEPENDS)
#	@if [ -d $(BUILD_DIR) ] && [ -z "$$(ls -A $(BUILD_DIR))" ]; then rm -rf $(BUILD_DIR); fi