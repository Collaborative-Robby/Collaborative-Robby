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

.PHONY: all clean plugins engine

all: $(BUILD_DIR) engine plugins

engine: $(BUILD_DIR) $(BUILD_DIR)/robby-engine

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/robby-engine: $(SRC_DIR)/robby.c $(INCLUDE_ROBBY_DIR)/struct.h
	$(CC) $(CFLAGS) $< -o $@

plugins: $(BUILD_DIR) $(addprefix $(BUILD_DIR)/,$(PLUGINS))

$(BUILD_DIR)/%: $(PLUGIN_SRC_DIR)/%.c $(INCLUDE_ROBBY_DIR)/module.h
	$(CC) $(MCFLAGS) $< -o $@.o
	$(CC) $(MCFLAGS_POST) -o $@ $@.o
	rm -f $@.o

clean:
	rm -rf $(BUILD_DIR)
