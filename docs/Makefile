# Sphinx make-mode entry point; uv preserves the caller's environment settings.
SPHINXOPTS ?=
SPHINXBUILD ?= uv run sphinx-build
SOURCEDIR = .
BUILDDIR = ../build/docs
.DEFAULT_GOAL := help
.PHONY: help html
help:
	$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS)
html:
	$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" -W $(SPHINXOPTS)
