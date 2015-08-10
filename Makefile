CC=gcc

SRC_DIR:=src
INCLUDE_DIR:=$(SRC_DIR)/include
INCLUDE_ROBBY_DIR:=$(INCLUDE_DIR)/robby

PLUGIN_SRC_DIR:=$(SRC_DIR)/plugins

CFLAGS=-I $(INCLUDE_DIR) -ldl -rdynamic
MCFLAGS:=-I $(INCLUDE_DIR) -fPIC -c
MCFLAGS_POST:=--shared

BUILD_DIR:=build

PLUGINS_SRC:=$(basename $(wildcard $(PLUGIN_SRC_DIR)/*))
PLUGINS:=$(notdir $(PLUGINS_SRC))

FIXED_COMMON=$(BUILD_DIR)/modules.o

.PHONY: all clean plugins engine

all: $(BUILD_DIR) engine plugins

engine: $(BUILD_DIR) $(BUILD_DIR)/robby-engine

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/robby-engine: $(SRC_DIR)/robby.c $(INCLUDE_ROBBY_DIR)/struct.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(MCFLAGS) $< -o $@

plugins: $(BUILD_DIR) $(addprefix $(BUILD_DIR)/,$(PLUGINS))

$(BUILD_DIR)/%: $(PLUGIN_SRC_DIR)/%.c $(FIXED_COMMON) $(INCLUDE_ROBBY_DIR)/module.h
	$(CC) $(MCFLAGS) $< -o $@.o
	$(CC) $(MCFLAGS_POST) $(FIXED_COMMON) -o $@ $@.o
	rm -f $@.o

clean:
	rm -rf $(BUILD_DIR)
