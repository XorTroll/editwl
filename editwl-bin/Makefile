
export ETWL_MAJOR	:=	0
export ETWL_MINOR	:=	1
export ETWL_MICRO	:=	0
export ETWL_BUGFIX	:=	0
export ETWL_VERSION	:=	$(ETWL_MAJOR).$(ETWL_MINOR).$(ETWL_MICRO).$(ETWL_BUGFIX)
export ETWL_OUTPUT_PATH	:=	$(CURDIR)/bin

###################################################################

.PHONY: all clean base libeditwl nedit

all: base libeditwl nedit

clean:
	@echo Cleaning libeditwl...
	@cmake --build libeditwl/build --target clean
	@echo Cleaning editwl...
	@cmake --build editwl/build --target clean
	@rm -rf bin

base:
	@mkdir -p $(ETWL_OUTPUT_PATH)

libeditwl:
	@echo Building libeditwl...
	@mkdir -p libeditwl/build
	@cmake -DCMAKE_BUILD_TYPE=Release -S libeditwl -B libeditwl/build
	@cmake --build libeditwl/build

nedit:
	@echo Building editwl...
	@mkdir -p editwl/build
	@cmake -DCMAKE_BUILD_TYPE=Release -S editwl -B editwl/build
	@cmake --build editwl/build
