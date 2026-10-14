"""Build project documentation without importing the native SystemC kernel."""
from pathlib import Path
import tomllib
from docutils import nodes

# -- Project information -----------------------------------------------------
_metadata = tomllib.loads((Path(__file__).resolve().parents[1] / "pyproject.toml").read_text())
project = "pysystemc — SystemC Python bindings"
author = "Amal Khailtash"
copyright = "2026, Amal Khailtash"
# Read the version locally so documentation builds work offline.
version = _metadata["project"]["version"]
release = version
rst_epilog = f"\n.. |project_version| replace:: {release}\n"
html_title = f"pysystemc {release} documentation"

# -- General configuration ---------------------------------------------------
extensions = ["sphinx.ext.mathjax", "sphinx.ext.napoleon",
              "sphinx.ext.githubpages", "myst_parser"]
exclude_patterns = ["_build"]
source_suffix = {".rst": "restructuredtext"}
napoleon_google_docstring = True

# -- HTML output -------------------------------------------------------------
html_theme = "sphinx_rtd_theme"
html_show_sourcelink = False
html_logo = "_static/logo.svg"
html_favicon = "_static/favicon.ico"
html_static_path = ["_static"]
html_css_files = ["css/custom.css"]
html_theme_options = {"collapse_navigation": False, "sticky_navigation": True,
                      "navigation_depth": 4, "includehidden": True,
                      "prev_next_buttons_location": "bottom"}
html_sidebars = {"**": ["globaltoc.html", "relations.html", "searchbox.html"]}


def refresh_package_badge(app, doctree, docname):
    """Give the PyPI badge a distinct cache URL for each documentation release."""
    for image in doctree.findall(nodes.image):
        if image.get("uri", "").startswith("https://img.shields.io/pypi/v/pysystemc?"):
            image["uri"] += f"&release={release}"


def setup(app):
    """Register the badge adjustment for local and hosted Sphinx builds."""
    app.connect("doctree-resolved", refresh_package_badge)
    return {"parallel_read_safe": True, "parallel_write_safe": True}
