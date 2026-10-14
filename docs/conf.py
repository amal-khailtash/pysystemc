"""Build project documentation without importing the native SystemC kernel."""
from pathlib import Path
import tomllib

# -- Project information -----------------------------------------------------
_metadata = tomllib.loads((Path(__file__).resolve().parents[1] / "pyproject.toml").read_text())
project = "pysystemc — SystemC Python bindings"
author = "Amal Khailtash"
copyright = "2026, Amal Khailtash"
# The local project metadata is authoritative; builds never query another
# package's PyPI release or require network access to determine the version.
version = _metadata["project"]["version"]
release = version

# -- General configuration ---------------------------------------------------
extensions = ["sphinx.ext.mathjax", "sphinx.ext.napoleon",
              "sphinx.ext.githubpages", "myst_parser"]
exclude_patterns = ["_build"]
source_suffix = {".rst": "restructuredtext"}
napoleon_google_docstring = True

# -- HTML output -------------------------------------------------------------
html_theme = "sphinx_rtd_theme"
html_show_sourcelink = False
html_static_path = ["_static"]
html_css_files = ["css/custom.css"]
html_theme_options = {"collapse_navigation": False, "sticky_navigation": True,
                      "navigation_depth": 4, "includehidden": True,
                      "prev_next_buttons_location": "bottom"}
html_sidebars = {"**": ["globaltoc.html", "relations.html", "searchbox.html"]}
