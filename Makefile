
run     := uv run
python  := $(run) python
lint    := $(run) pylint
pyright := $(run) pyright
black   := $(run) black
twine   := $(run) twine
ruff    := $(run) ruff


# help: ## This help.
# 	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST) | sort

.PHONY: help
help:  ## Display this help screen
	@echo -e "\033[1mAvailable commands:\033[0m"
	@grep -E '^[a-z.A-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-22s\033[0m %s\n", $$1, $$2}' | sort


.PHONY: uv
uv:
	@curl -LsSf https://astral.sh/uv/install.sh | sh


UV:=$(shell uv --version)
ifdef UV
	VENV := uv venv
	PIP  := uv pip
else
	VENV := python -m venv
	PIP  := python -m pip
endif


.venv:
	PYTHON_HOME=/tool/pandora64/.package/miniforge3-3.12.5/ $(VENV) .venv

venv: .venv
	@echo "run 'source .venv/bin/activate' to use virtualenv"


.PHONY: uv-lock
uv-lock:
	uv lock

.PHONY: uv-sync
uv-sync:
	uv sync



cppyy:
	CC=/tool/pandora64/.package/gcc-10.2.0/bin/gcc  \
	CXX=/tool/pandora64/.package/gcc-10.2.0/bin/g++ \
	LD=/tool/pandora64/.package/gcc-10.2.0/bin/g++ \
	STDCXX=14                                       \
	MAKE_NPROCS=16                                  \
	uv add cppyy
#	pip install --force-reinstall cppyy


# git-submodules:
# 	git submodule add https://github.com/accellera-official/systemc submodules/systemc
# 	(cd submodules/systemc/; git checkout 2.3.4)

git-submodules:
	@git submodule update --init --recursive
	@( \
		cd submodules/systemc/ ; \
		git checkout 2.3.4     ; \
		CC=/tool/pandora64/.package/gcc-10.2.0/bin/gcc  \
		CXX=/tool/pandora64/.package/gcc-10.2.0/bin/g++ \
		LD=/tool/pandora64/.package/gcc-10.2.0/bin/g++ \
		./configure --prefix=$(PWD)/submodules/systemc ; \
		make -j16 install; \
	)


# git submodule update --init --remote --recursive
# git submodule update --recursive
# git clone --recursive


# CC=/tool/pandora64/.package/gcc-10.2.0/bin/gcc \
# CXX=/tool/pandora64/.package/gcc-10.2.0/bin/g++ \
# LD=/tool/pandora64/.package/gcc-10.2.0/bin/ld \
# STDCXX=14 MAKE_NPROCS=16 pip install cppyy

hello-cpp:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/run_sysc_cpp.py examples/helloworld/cpp/helloworld.cpp


hello-py:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/helloworld/python/helloworld.py


simple_fifo-py:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/simple_fifo/python/simple_fifo.py

simple_fifo-cpp:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc \
		uv run examples/run_sysc_cpp.py     \
			submodules/systemc/examples/sysc/simple_fifo/simple_fifo.cpp


pipe-cpp:
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


pkt_switch-cpp:
	@SYSTEMC_HOME=$(PWD)/submodules/systemc                            \
		uv run examples/run_sysc_cpp.py                                \
			submodules/systemc/examples/sysc/pkt_switch/fifo.cpp       \
			submodules/systemc/examples/sysc/pkt_switch/sender.cpp     \
			submodules/systemc/examples/sysc/pkt_switch/switch_clk.cpp \
			submodules/systemc/examples/sysc/pkt_switch/switch.cpp     \
			submodules/systemc/examples/sysc/pkt_switch/receiver.cpp   \
			submodules/systemc/examples/sysc/pkt_switch/main.cpp