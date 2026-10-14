Publishing
==========

Push with an encrypted SSH key
------------------------------

Unlock your key in your terminal. Enter its passphrase at the SSH prompt; do not
put it in project files or send it to an assistant::

   eval "$(ssh-agent -s)"
   ssh-add
   git push origin main

Use ``ssh-add /path/to/key`` if your key has a nondefault filename.

PyPI and TestPyPI
-----------------

The GitHub ``Publish Python distributions`` workflow builds with uv, saves the
wheel and sdist as artifacts, and publishes through trusted publishing. It runs
only when manually dispatched; TestPyPI is the default destination.

Before using it:

* Confirm that the distribution name ``systemc`` is available to your account.
* Configure a trusted publisher on PyPI and TestPyPI for repository
  ``amal-khailtash/pysystemc``, workflow ``publish.yml``, and environment ``pypi``
  or ``testpypi`` respectively.
* Create those GitHub environments and add required reviewers for publication.
* Update the version in ``pyproject.toml`` and move reviewed changelog entries
  into a dated release section. PyPI versions cannot be overwritten.
* Run setup, tests, documentation and artifact installation checks. Publish to
  TestPyPI first, then manually select PyPI after reviewing the artifacts.

The wheel contains Python and JIT bridge sources, not a bundled SystemC library.
Users still need compatible SystemC 2.3.4 pthread libraries, headers, kernel entry
source and fmt headers. Clean installation outside this checkout remains to be
validated before the first public release. Registry cppyy replaces the Git
requirement because PyPI does not accept direct URL dependencies.

Read the Docs
-------------

Import this GitHub repository in Read the Docs and enable the default branch and
release tags you want documented. ``.readthedocs.yaml`` selects Python 3.14 and
installs only the uv documentation group. Documentation builds do not import the
native kernel or build SystemC. Set the project slug to ``pysystemc`` or update the
documentation URL in ``pyproject.toml`` if that slug is unavailable.
