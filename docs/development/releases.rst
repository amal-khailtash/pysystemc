Pull requests and releases
=========================

Workflow order
--------------

Both repositories use the same four Actions workflow names. The numbers make
navigation consistent; jobs run according to their triggers and dependencies.

* **01. PR title** (``pr-title.yml``) checks Conventional Commit PR titles using
  metadata only.
* **02. CI** (``ci.yml``) tests, checks distributions, and builds documentation
  for PRs and main. The publisher calls the same CI against the release tag.
* **03. Release Please** (``release-please.yml``) collects merged changes into a
  release PR. Merging that PR creates the immutable tag and GitHub release and
  calls the publisher directly.
* **04. Publish** (``publish.yml``) verifies the tag/version and main ancestry,
  publishes the distributions produced by release CI, then triggers released
  documentation on Read the Docs. Manual dispatch accepts an existing tag and
  a ``pypi`` or ``testpypi`` destination for recovery or testing.

Read the Docs handles PR previews and development ``latest`` docs through its
GitHub integration. CI's documentation build validates HTML; it does not deploy
it. Disable independent automatic tag builds in Read the Docs when strict
PyPI-before-release-docs ordering is required. Keep tag versions discoverable
and active so the post-publication API trigger can build them. A successful
trigger means the build was queued; inspect Read the Docs for its final result.
GitHub Pages and the separate TestPyPI workflow are removed.

Contributor and release flow
----------------------------

Use ``<type>/<short-description>`` for contributor branches in both repositories:
``feat/...``, ``fix/...``, ``ci/...``, ``docs/...``, or ``test/...``. Use lowercase
words separated by hyphens, for example ``ci/consolidate-workflows``. Target
``main`` and use the same branch name when one change spans both repositories.
Release Please owns its generated ``release-please--branches--main--components--<package>``
branch; let it manage that name rather than creating manual release branches.

Work on a branch, open a PR, and wait for title, Tests and coverage, Build
distributions, and Build documentation checks to pass. Squash-merge with the
Conventional Commit PR title and enable automatic deletion of merged branches.
Require those checks in main protection. A sole maintainer may use zero required
reviews; require a review when another reviewer is available.

Contributors do not edit versions or create tags. Release Please manages
``pyproject.toml``, ``uv.lock``, the manifest, and ``CHANGELOG.md``. ``fix:`` and
``perf:`` request a patch; ``feat:`` requests a minor; ``feat!:`` or a
``BREAKING CHANGE:`` footer requests a major, including below 1.0. CI, docs,
tests, and chores alone do not request releases. Review the generated release
PR and its checks before merging.

PyPI versions and tags are immutable. Inspect uploaded files before retrying a
partial publication. Run ``04. Publish`` from main with the existing tag to use
workflow repairs without changing released source. No new version or tag is
needed for a workflow-only repair.

Service setup
-------------

* Enable Actions PR creation and configure ``RELEASE_PLEASE_TOKEN`` with contents
  and pull-request write access. Its PRs must trigger normal CI.
* Configure trusted publishers for this repository's ``publish.yml`` and the
  ``pypi`` / ``testpypi`` environments. Replace any TestPyPI publisher entry that
  still names the removed ``publish-testpypi.yml``. Workflow filenames remain
  stable even though their display names are numbered.
* Configure ``CODECOV_TOKEN`` and the Read the Docs GitHub integration for
  coverage and PR previews.
* Configure ``READTHEDOCS_TOKEN`` with permission to trigger this project's
  released version builds. The publisher uses it only after a successful PyPI
  upload; TestPyPI does not trigger released docs.

Project-specific validation
---------------------------

pysystemc builds and repairs native manylinux wheels with auditwheel. Before
upload it checks native resources, runtime compilation, pthread waits, simulation
time, console arguments, and tutorials using the installed wheel outside the
build container. Python 3.14 remains required.
