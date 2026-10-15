MAKEFILE_DIR   = $(patsubst %/,%,$(dir $(realpath $(firstword $(MAKEFILE_LIST)))))

-include $(MAKEFILE_DIR)/Makefile-helper.mk

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Resolve the active environment's header suffix only when a recipe needs it.
# Help and setup must also work before the virtual environment exists.
PYTHON_VERSION = $(shell .venv/bin/python -c "import sys; print(str(sys.version_info.major) + '.' + str(sys.version_info.minor))")

# Published-package smoke checks default to the version declared in pyproject.toml.
# Select another release with: make test-testpypi PACKAGE_VERSION=0.2.1
PACKAGE_VERSION ?= $(shell awk '/^version = / {gsub(/"/, "", $$3); print $$3; exit}' $(MAKEFILE_DIR)/pyproject.toml)

# Release builds may use: uv build --out-dir dist/$(PACKAGE_VERSION).
WHEEL_DIR ?= $(MAKEFILE_DIR)/dist/$(PACKAGE_VERSION)

VENV := uv venv

# CMake output locations and coroutine backend can be overridden on the command line.
# Python callbacks require pthread stacks on Python 3.14. Both builders use it.
SYSTEMC_BUILD_DIR ?= $(MAKEFILE_DIR)/build/deps/systemc
SYSTEMC_INSTALL_DIR ?= $(MAKEFILE_DIR)/build/deps/systemc-install
export SYSTEMC_INSTALL_DIR

SYSTEMC_PTHREADS ?= ON
# Pinned upstream release shared by both native build paths.
SYSTEMC_VERSION := 2.3.4
BUILD_JOBS ?= 8

FMT_VERSION := 11.2.0
FMT_HOME ?= $(MAKEFILE_DIR)/build/deps/fmt-$(FMT_VERSION)
export FMT_HOME
SPDLOG_VERSION := 1.15.3
SPDLOG_HOME ?= $(MAKEFILE_DIR)/build/deps/spdlog-$(SPDLOG_VERSION)
export SPDLOG_HOME

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
# setup: ## Set up Python dependencies, CPyCppyy headers, fmt/spdlog, and pthreads SystemC 2.3.4
# ----------------------------------------------------------------------------------------------------------------------
# Recursive recipes enforce order even when the caller uses make -j.
# uv sync creates the environment and honors the caller's UV_* settings.
.PHONY: setup
setup:
	$(MAKE) uv-sync
	$(MAKE) fix-cppyy
	$(MAKE) setup-fmt
	$(MAKE) setup-spdlog
	$(MAKE) setup-systemc-cmake

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

# ----------------------------------------------------------------------------------------------------------------------
# add-cppyy: ## Add cppyy to the project
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: add-cppyy
add-cppyy:
	STDCXX=17      \
	MAKE_NPROCS=16 \
	uv add --upgrade 'cppyy @ git+https://github.com/wlav/cppyy'
#	uv add cppyy

# STDCXX=14 uv add cppyy --upgrade 'cppyy @ git+https://github.com/wlav/cppyy'

# ----------------------------------------------------------------------------------------------------------------------
# fix-cppyy: ## Repair the CPyCppyy header alias in the active virtual environment
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: fix-cppyy
fix-cppyy:
	(cd .venv/include/site/python$(PYTHON_VERSION); test -e CPyCppyy/API.h || ln -s cpycppyy CPyCppyy)


# ----------------------------------------------------------------------------------------------------------------------
# remove-cppyy: ## Remove cppyy to the project
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: remove-cppyy
remove-cppyy:
	uv cache clean
	uv remove cppyy

# ----------------------------------------------------------------------------------------------------------------------
# check-systemc-version: ## Initialize the pinned submodule and verify SystemC 2.3.4
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: check-systemc-version
check-systemc-version:
	@test -f submodules/systemc/configure || git submodule update --init --recursive
	@test "$$(git -C submodules/systemc rev-parse HEAD)" = "$$(git ls-files --stage submodules/systemc | awk '{print $$2}')" || { echo "SystemC checkout differs from the pinned submodule; restore it before building." >&2; exit 1; }
	@test "$$(awk '/^#define SC_VERSION_(MAJOR|MINOR|PATCH) / {version = version separator $$3; separator = "."} END {print version}' submodules/systemc/src/sysc/kernel/sc_ver.h)" = "$(SYSTEMC_VERSION)" || { echo "Expected SystemC $(SYSTEMC_VERSION); restore the pinned submodule before building." >&2; exit 1; }

# ----------------------------------------------------------------------------------------------------------------------
# setup-systemc: ## Build and install SystemC with Autotools and pthreads
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: setup-systemc
setup-systemc: check-systemc-version
	@( \
		set -e; \
		cd submodules/systemc/ ; \
		./configure --enable-pthreads --prefix=$(SYSTEMC_INSTALL_DIR) --libdir=$(SYSTEMC_INSTALL_DIR)/lib CXXFLAGS="-DSC_CPLUSPLUS=201703L -std=c++17" ; \
		make -j$(BUILD_JOBS) install; \
	)

# ----------------------------------------------------------------------------------------------------------------------
# setup-systemc-cmake: ## Build and install SystemC with CMake and pthreads
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: setup-systemc-cmake
setup-systemc-cmake: check-systemc-version
	cmake -S submodules/systemc -B $(SYSTEMC_BUILD_DIR) -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_CXX_STANDARD=17 -DENABLE_PTHREADS=$(SYSTEMC_PTHREADS) -DCMAKE_INSTALL_PREFIX=$(SYSTEMC_INSTALL_DIR) -DCMAKE_INSTALL_LIBDIR=lib
	cmake --build $(SYSTEMC_BUILD_DIR) -j$(BUILD_JOBS)
	cmake --install $(SYSTEMC_BUILD_DIR)

# ----------------------------------------------------------------------------------------------------------------------
# setup-fmt: ## Download pinned fmt headers for binding diagnostics (override FMT_HOME for an existing installation)
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: setup-fmt
setup-fmt:
	mkdir -p $(MAKEFILE_DIR)/build/deps
	curl -fL https://github.com/fmtlib/fmt/archive/refs/tags/$(FMT_VERSION).tar.gz -o $(MAKEFILE_DIR)/build/deps/fmt-$(FMT_VERSION).tar.gz
	tar -xzf $(MAKEFILE_DIR)/build/deps/fmt-$(FMT_VERSION).tar.gz -C $(MAKEFILE_DIR)/build/deps

# ----------------------------------------------------------------------------------------------------------------------
# setup-spdlog: ## Download pinned spdlog headers for fmt-based C++ runner logging
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: setup-spdlog
setup-spdlog:
	mkdir -p $(MAKEFILE_DIR)/build/deps
	curl -fL https://github.com/gabime/spdlog/archive/refs/tags/v$(SPDLOG_VERSION).tar.gz -o $(MAKEFILE_DIR)/build/deps/spdlog-$(SPDLOG_VERSION).tar.gz
	tar -xzf $(MAKEFILE_DIR)/build/deps/spdlog-$(SPDLOG_VERSION).tar.gz -C $(MAKEFILE_DIR)/build/deps

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
##@ Build and validation:
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ----------------------------------------------------------------------------------------------------------------------
# build: ## Build native dependencies, wheel and source distribution using uv
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: build
build:
	uv build

# ----------------------------------------------------------------------------------------------------------------------
# test: ## Run all subprocess regressions using the automatically discovered SystemC library
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: test
test:
	SYSTEMC_HOME=$(MAKEFILE_DIR)/submodules/systemc uv run pytest -q

# ----------------------------------------------------------------------------------------------------------------------
# test-verbose: ## Run all subprocess regressions with verbose pytest results
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: test-verbose
test-verbose:
	SYSTEMC_HOME=$(MAKEFILE_DIR)/submodules/systemc uv run pytest -vv

# ----------------------------------------------------------------------------------------------------------------------
# lint: ## Check first-party Python with Ruff (Python 3.14)
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: lint
lint:
	uv run ruff check .

# ----------------------------------------------------------------------------------------------------------------------
# format-python: ## Format first-party Python with Ruff
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: format-python
format-python:
	uv run ruff format .

# ----------------------------------------------------------------------------------------------------------------------
# coverage: ## Run regression coverage with terminal and HTML reports (build/coverage/html)
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: coverage
coverage:
	SYSTEMC_HOME=$(MAKEFILE_DIR)/submodules/systemc uv run pytest --cov=amal.eda.systemc --cov-report=term-missing --cov-report=html:build/coverage/html --cov-report=xml:build/coverage/coverage.xml

# ----------------------------------------------------------------------------------------------------------------------
# native-sanitize: ## Run optional ASan/UBSan first-party native references
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: native-sanitize
native-sanitize:
	uv run python tools/native_sanitize.py

# ----------------------------------------------------------------------------------------------------------------------
# test-wheel: ## Check installed wheel startup, console and tutorials (override PACKAGE_VERSION and WHEEL_DIR)
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: test-wheel
test-wheel:
	@set -e; \
	wheel=$$(find "$(WHEEL_DIR)" -maxdepth 1 -name 'pysystemc-$(PACKAGE_VERSION)-*.whl' -print -quit); \
	test -n "$$wheel" || { echo "No wheel for $(PACKAGE_VERSION) in $(WHEEL_DIR); run uv build --out-dir $(WHEEL_DIR)" >&2; exit 1; }; \
	wheel=$$(realpath "$$wheel"); \
	cd /tmp; \
	env -u FMT_HOME -u SPDLOG_HOME -u SYSTEMC_HOME -u SYSTEMC_SOURCE_DIR -u SYSTEMC_INSTALL_DIR -u SYSTEMC_LIBRARY_DIR \
		uv run --no-project --with "$$wheel" \
		python "$(MAKEFILE_DIR)/tools/check_published.py" "$(PACKAGE_VERSION)"; \
	env -u FMT_HOME -u SPDLOG_HOME -u SYSTEMC_HOME -u SYSTEMC_SOURCE_DIR -u SYSTEMC_INSTALL_DIR -u SYSTEMC_LIBRARY_DIR \
		uv run --no-project --with "$$wheel" \
		python "$(MAKEFILE_DIR)/tools/check_installed_examples.py"

# ----------------------------------------------------------------------------------------------------------------------
# test-testpypi: ## Check installed TestPyPI release (override PACKAGE_VERSION to select a version)
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: test-testpypi
test-testpypi:
	cd /tmp && env -u FMT_HOME -u SPDLOG_HOME -u SYSTEMC_HOME -u SYSTEMC_SOURCE_DIR -u SYSTEMC_INSTALL_DIR -u SYSTEMC_LIBRARY_DIR \
		uv run --no-project --refresh-package pysystemc --index https://test.pypi.org/simple --index-strategy unsafe-best-match \
		--with 'pysystemc==$(PACKAGE_VERSION)' \
		python "$(MAKEFILE_DIR)/tools/check_published.py" "$(PACKAGE_VERSION)"

# ----------------------------------------------------------------------------------------------------------------------
# test-pypi: ## Check installed PyPI release (override PACKAGE_VERSION to select a version)
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: test-pypi
test-pypi:
	cd /tmp && env -u FMT_HOME -u SPDLOG_HOME -u SYSTEMC_HOME -u SYSTEMC_SOURCE_DIR -u SYSTEMC_INSTALL_DIR -u SYSTEMC_LIBRARY_DIR \
		uv run --no-project --refresh-package pysystemc --with 'pysystemc==$(PACKAGE_VERSION)' \
		python "$(MAKEFILE_DIR)/tools/check_published.py" "$(PACKAGE_VERSION)"

# ----------------------------------------------------------------------------------------------------------------------
# docs: ## Build Sphinx HTML documentation with warnings treated as errors
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: docs
docs:
	uv run sphinx-build -W -b html docs build/docs

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
##@ Examples:
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ----------------------------------------------------------------------------------------------------------------------
# example-adder-py: ## Verify parameterized unsigned addition and overflow
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-adder-py
example-adder-py:
	uv run examples/toy/adder.py

# ----------------------------------------------------------------------------------------------------------------------
# example-up-counter-py: ## Verify eight-bit reset, enable and wraparound
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-up-counter-py
example-up-counter-py:
	uv run examples/toy/up_counter.py

# ----------------------------------------------------------------------------------------------------------------------
# example-export-py: ## Run hierarchical native interface exports from Python
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-export-py
example-export-py:
	uv run examples/sysc/2.1/sc_export/main.py

# ----------------------------------------------------------------------------------------------------------------------
# example-trace-cleanup-py: ## Verify VCD closure and transitions after an exception
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-trace-cleanup-py
example-trace-cleanup-py:
	uv run examples/toy/trace_cleanup.py

# ----------------------------------------------------------------------------------------------------------------------
# example-event-cancel-py: ## Verify event cancellation and initialization control
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-event-cancel-py
example-event-cancel-py:
	uv run examples/toy/event_cancel.py

# ----------------------------------------------------------------------------------------------------------------------
# example-mixed-gate-py: ## Drive a native C++ gate from Python signals
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-mixed-gate-py
example-mixed-gate-py:
	uv run examples/toy/mixed_gate.py

# ----------------------------------------------------------------------------------------------------------------------
# example-specialized-signals-py: ## Verify upstream integer signal concatenation and arithmetic
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-specialized-signals-py
example-specialized-signals-py:
	uv run examples/sysc/2.1/specialized_signals/main.py

# ----------------------------------------------------------------------------------------------------------------------
# example-barrier-py: ## Run the upstream Python event barrier example
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-barrier-py
example-barrier-py:
	uv run examples/sysc/2.1/scx_barrier/main.py

# ----------------------------------------------------------------------------------------------------------------------
# example-forkjoin-py: ## Run the upstream Python dynamic process and join example
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-forkjoin-py
example-forkjoin-py:
	uv run examples/sysc/2.1/forkjoin/forkjoin.py

# ----------------------------------------------------------------------------------------------------------------------
# example-report-py: ## Run the upstream Python report action-rule example
# ----------------------------------------------------------------------------------------------------------------------
.PHONY: example-report-py
example-report-py:
	uv run examples/sysc/2.1/sc_report/main.py

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
	CPPYY_API_PATH=$(PWD)/.venv/include/site/python$(PYTHON_VERSION) \
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
example-pipe-py:
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
