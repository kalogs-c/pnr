default: run

# You can change these variables if you want to use a different compiler or debugger
CC = clang
DBG = lldb

build-main: build-dir ## Build main
	$(CC) -Wall -O3 -g -o build/main -ledit -lm main.c mpc.c lval.c

check: ## Check prerequisites (compiler and debugger)
	@which $(CC) > /dev/null && echo "SUCCESS: $(CC) is installed" || echo "ERROR: $(CC) not found, please install clang"
	@which $(DBG) > /dev/null && echo "SUCCESS: $(DBG) is installed" || echo "ERROR: $(DBG) not found, please install lldb"

build-dir: ## Create build directory
	if [ ! -d build ]; then mkdir build; fi

run: build-main ## Run main.c
	./build/main

debug: build-main ## Debug main.c
	$(DBG) ./build/main

help:  ## Display this help
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make \033[36m\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

