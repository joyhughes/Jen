MAKEFLAGS += --no-builtin-rules

# Directories
ROOT_DIR := $(shell pwd)
EXTERN_DIR := $(ROOT_DIR)/external
EXTERN_BUILD_DIR := $(EXTERN_DIR)/build
EXTERN_LIB_DIR := $(EXTERN_DIR)/lib
VPX_DIR := $(EXTERN_DIR)/vpx
FFMPEG_DIR := $(EXTERN_DIR)/FFmpeg
SRC_DIR := $(ROOT_DIR)/src
WEB_BUILD_DIR := $(ROOT_DIR)/web_build
SCRIPT_PATH := $(ROOT_DIR)/build_jen.sh

# Main targets
.PHONY: all clean deep-clean run-script

all: run-script

# Run the all-in-one build script
run-script:
	@if [ ! -f $(SCRIPT_PATH) ]; then \
		echo "Creating build script..."; \
		cp $(ROOT_DIR)/all_in_one_script.sh $(SCRIPT_PATH); \
		chmod +x $(SCRIPT_PATH); \
	fi
	@echo "Running build script..."
	@$(SCRIPT_PATH)

# Clean target
clean:
	rm -f $(WEB_BUILD_DIR)/*.o $(WEB_BUILD_DIR)/*.d lux_react/src/lux.js
	@echo "Cleaned build files"

# Deep clean (removes external dependencies too)
deep-clean: clean
	rm -rf $(EXTERN_BUILD_DIR)/lib/*.a $(EXTERN_LIB_DIR)/*.a
	@echo "Removed library files"