CC=gcc

SRC_DIR:=src
INCLUDE_DIR:=$(SRC_DIR)/include
INCLUDE_ROBBY_DIR:=$(INCLUDE_DIR)/robby

PLUGIN_SRC_DIR:=$(SRC_DIR)/plugins

CFLAGS=-I $(INCLUDE_DIR) -ldl
MCFLAGS:=-I $(INCLUDE_DIR) -fPIC -c
MCFLAGS_POST:=--shared

BUILD_DIR:=build

PLUGINS_SRC:=$(basename $(wildcard $(PLUGIN_SRC_DIR)/*))
PLUGINS:=$(notdir $(PLUGINS_SRC))

.PHONY: all clean plugins

all: $(BUILD_DIR) robby-engine plugins

$(BUILD_DIR):
	mkdir $@

robby-engine: $(SRC_DIR)/robby.c $(INCLUDE_ROBBY_DIR)/struct.h
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/$@

plugins: $(PLUGINS)

%: $(PLUGIN_SRC_DIR)/%.c $(INCLUDE_ROBBY_DIR)/module.h
	$(CC) $(MCFLAGS) $< -o $@.o
	$(CC) $(MCFLAGS_POST) -o $(BUILD_DIR)/$@ $@.o
	rm $@.o

clean:
	rm $(BUILD_DIR)
