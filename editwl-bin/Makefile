
export ETWL_MAJOR	:=	0
export ETWL_MINOR	:=	1
export ETWL_MICRO	:=	0
export ETWL_BUGFIX	:=	0
export ETWL_VERSION	:=	$(ETWL_MAJOR).$(ETWL_MINOR).$(ETWL_MICRO).$(ETWL_BUGFIX)
export ETWL_MODULES_DIR	:=	modules
export ETWL_OUTPUT_PATH	:=	$(CURDIR)/bin
export ETWL_OUTPUT_MODULES_PATH	:=	$(ETWL_OUTPUT_PATH)/modules

BUILTIN_MODULES	:=	BMG ROM

###################################################################

BUILTIN_MODULE_BUILD_TARGETS	:=	$(foreach target,$(BUILTIN_MODULES),$(target).build)
BUILTIN_MODULE_CLEAN_TARGETS	:=	$(foreach target,$(BUILTIN_MODULES),$(target).clean)

.PHONY: all clean base libeditwl nedit

all: base libeditwl nedit $(BUILTIN_MODULE_BUILD_TARGETS)

clean: $(BUILTIN_MODULE_CLEAN_TARGETS)
	@echo Cleaning libeditwl...
	@cmake --build libeditwl/build --target clean
	@echo Cleaning editwl-bin...
	@cmake --build editwl-bin/build --target clean
	@rm -rf bin

base:
	@mkdir -p $(ETWL_OUTPUT_PATH)
	@mkdir -p $(ETWL_OUTPUT_MODULES_PATH)

libeditwl:
	@echo Building libeditwl...
	@mkdir -p libeditwl/build
	@cmake -DCMAKE_BUILD_TYPE=Release -S libeditwl -B libeditwl/build
	@cmake --build libeditwl/build

nedit:
	@echo Building editwl-bin...
	@mkdir -p editwl-bin/build
	@cmake -DCMAKE_BUILD_TYPE=Release -S editwl-bin -B editwl-bin/build
	@cmake --build editwl-bin/build

%.build:
	@echo Building builtin module '$(basename $@)'...
	@mkdir -p modules/$(basename $@)/build
	@cmake -DCMAKE_BUILD_TYPE=Release -S modules/$(basename $@) -B modules/$(basename $@)/build
	@cmake --build modules/$(basename $@)/build

%.clean:
	@echo Cleaning builtin module '$(basename $@)'...
	@cmake --build modules/$(basename $@)/build --target clean
