MAKEFILE_DIR   = $(patsubst %/,%,$(dir $(realpath $(firstword $(MAKEFILE_LIST)))))

-include $(MAKEFILE_DIR)/Makefile-helper.mk

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

UV:=$(shell uv --version)
ifdef UV
	VENV := uv venv
	PIP  := uv pip
else
	VENV := python -m venv
	PIP  := python -m pip
endif

run     := uv run
python  := $(run) python
lint    := $(run) pylint
test    := $(run) pytest
pyright := $(run) pyright
black   := $(run) black
ruff    := $(run) ruff

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## .PHONY: help
## help: ## This help.
## 	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST) | sort

## .PHONY: help
## help:  ## Display this help screen
## 	@echo -e "\033[1mAvailable commands:\033[0m"
## 	@grep -E '^[a-z.A-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-22s\033[0m %s\n", $$1, $$2}' | sort

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
##@ Setup:
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ----------------------------------------------------------------------------------------------------------------------
# uv: ## Install uv
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: uv
uv:
	@curl -LsSf https://astral.sh/uv/install.sh | sh

# ----------------------------------------------------------------------------------------------------------------------
# venv: ## Create a virtual environment
# ----------------------------------------------------------------------------------------------------------------------
.venv:
	$(VENV) .venv

venv: .venv
	@echo "run 'source .venv/bin/activate' to use virtualenv"

# ----------------------------------------------------------------------------------------------------------------------
# uv-lock: ## Create a lockfile for the project's dependencies.
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: uv-lock
uv-lock:
	uv lock

# ----------------------------------------------------------------------------------------------------------------------
# uv-sync: ## Sync the project's dependencies with the environment.
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: uv-sync
uv-sync:
	uv sync

## # ----------------------------------------------------------------------------------------------------------------------
## # add-cppyy: ## Add cppyy to the project
## # ----------------------------------------------------------------------------------------------------------------------
## .PHONY: add-cppyy
## add-cppyy:
## 	STDCXX=14      \
## 	MAKE_NPROCS=16 \
## 	uv add cppyy

# ----------------------------------------------------------------------------------------------------------------------
# setup-systemc: ## Fetch and install SystemC submodule
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: setup-systemc
setup-systemc:	## Fetch and install SystemC submodule
	@git submodule update --init --recursive
	@( \
		cd submodules/systemc/ ; \
		git checkout 2.3.4     ; \
		./configure --prefix=$(PWD)/submodules/systemc ; \
		make -j16 install; \
	)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
##@ Examples:
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ----------------------------------------------------------------------------------------------------------------------
# example-hello-cpp: ## Run the hello world example in C++
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-hello-cpp
example-hello-cpp:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/run_sysc_cpp.py \
			examples/helloworld/cpp/helloworld.cpp

# ----------------------------------------------------------------------------------------------------------------------
# example-hello-py: ## Run the hello world example in Python
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-hello-py
example-hello-py:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/helloworld/py/helloworld.py

# ----------------------------------------------------------------------------------------------------------------------
# example-counter-cpp: ## Run the counter example in C++
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-counter-cpp
example-counter-cpp:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/run_sysc_cpp.py \
			examples/counter/cpp/counter_tb.cpp

# ----------------------------------------------------------------------------------------------------------------------
# example-counter-py: ## Run the counter example in Python
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-counter-py
example-counter-py:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/counter/py/counter_tb.py

# ----------------------------------------------------------------------------------------------------------------------
# example-simple_fifo-py: ## Run the simple_fifo example in Python
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-simple_fifo-py
example-simple_fifo-py:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/simple_fifo/py/simple_fifo.py

# ----------------------------------------------------------------------------------------------------------------------
# example-pipe-cpp: ## Run the pipe example in C++
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-pipe-cpp
example-pipe-cpp:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc                   \
		uv run examples/run_sysc_cpp.py                       \
			submodules/systemc/examples/sysc/pipe/display.h   \
			submodules/systemc/examples/sysc/pipe/display.cpp \
			submodules/systemc/examples/sysc/pipe/numgen.h    \
			submodules/systemc/examples/sysc/pipe/numgen.cpp  \
			submodules/systemc/examples/sysc/pipe/stage1.h    \
			submodules/systemc/examples/sysc/pipe/stage1.cpp  \
			submodules/systemc/examples/sysc/pipe/stage2.h    \
			submodules/systemc/examples/sysc/pipe/stage2.cpp  \
			submodules/systemc/examples/sysc/pipe/stage3.h    \
			submodules/systemc/examples/sysc/pipe/stage3.cpp  \
			submodules/systemc/examples/sysc/pipe/main.cpp

# ----------------------------------------------------------------------------------------------------------------------
# example-pipe-py: ## Run the pipe example in Python
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-pipe-py
example-simple_fifo-cpp:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/run_sysc_cpp.py     \
			submodules/systemc/examples/sysc/simple_fifo/simple_fifo.cpp

# ----------------------------------------------------------------------------------------------------------------------
# example-pkt_switch-cpp: ## Run the pkt_switch example in C++
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-pkt_switch-cpp
example-pkt_switch-cpp:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc                            \
		uv run examples/run_sysc_cpp.py                                \
			submodules/systemc/examples/sysc/pkt_switch/fifo.cpp       \
			submodules/systemc/examples/sysc/pkt_switch/sender.cpp     \
			submodules/systemc/examples/sysc/pkt_switch/switch_clk.cpp \
			submodules/systemc/examples/sysc/pkt_switch/switch.cpp     \
			submodules/systemc/examples/sysc/pkt_switch/receiver.cpp   \
			submodules/systemc/examples/sysc/pkt_switch/main.cpp

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
##@ General:
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ----------------------------------------------------------------------------------------------------------------------
# vars: ## Print the vars used in the Makefile
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: vars
vars:
	@echo -e "$(BOLD)MAKEFILE_DIR$(RESET) : $(MAKEFILE_DIR)"
	@echo -e "$(BOLD)BUILD_TYPE$(RESET)   : $(STRATUS_HOME)"

# ----------------------------------------------------------------------------------------------------------------------
# var-<VARIABLE>: ## Print single variable used in the Makefile
# ----------------------------------------------------------------------------------------------------------------------
var-%:
	@echo -e '$(BOLD)$*$(RESET)=$($*)'