Pull requests and releases
==========================

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
  dispatches the publisher from main so its trusted publisher identity matches.
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

Branch naming
-------------

Both projects use ``<type>/<short-description>``. Choose a prefix describing the
work and lowercase words separated by hyphens:

.. list-table:: Contributor branch examples
   :header-rows: 1
   :widths: 15 40 45

   * - Prefix
     - Example
     - Purpose
   * - ``feat``
     - ``feat/custom-interfaces``
     - New public behavior
   * - ``fix``
     - ``fix/version-validation``
     - Correct existing behavior
   * - ``ci``
     - ``ci/consolidate-workflows``
     - Automation and workflow changes
   * - ``docs``
     - ``docs/development-workflow``
     - Documentation changes
   * - ``test``
     - ``test/interface-lifetime``
     - Test coverage

Use ``build``, ``perf``, or ``refactor`` when those Conventional Commit types
better describe the change. Branch names organize work; the squash commit's PR
title determines release impact. For example, ``ci/consolidate-workflows`` with
PR title ``ci: consolidate numbered release workflows`` requests no release.

Create branches from current ``main`` and target PRs at ``main``. For changes
spanning both repositories, use the same branch name in each, but open a separate
PR in each repository: the branches and merges remain independent. For example::

   git switch main
   git pull --ff-only
   git switch -c ci/consolidate-workflows

Release Please manages ``release-please--branches--main--components--<package>``;
``<package>`` is ``pysystemc`` or ``pyipxact-de``. These generated release PR
branches have different component suffixes because they release different
packages. Contributors do not create manual version/release branches or tags.

PR workflow
-----------

1. Implement and validate the change on its contributor branch. Update relevant
   docs and changelog notes, commit, push, and open a PR with a Conventional
   Commit title, such as ``feat: support custom interfaces``.
2. ``01. PR title`` checks the title; ``02. CI`` runs tests, builds distributions,
   and validates docs. Read the Docs supplies a separate PR preview.
3. Review the result and wait for title, Tests and coverage, Build distributions,
   and Build documentation checks to pass. Require these checks in main
   protection. A sole maintainer may use zero required reviews; require a review
   when another reviewer is available.
4. Squash-merge using the PR title. Main receives one commit containing the PR's
   changes. Individual branch commit IDs are replaced by the squash commit ID;
   this is expected and does not remove the merged files.
5. GitHub deletes the merged remote branch when automatic deletion is enabled.
   Update local main and delete the completed local branch after confirming its
   changes are present. Squash commits may prevent ``git branch -d`` from
   recognizing ancestry; inspect the merged PR before using ``git branch -D``.

Release workflow
----------------

1. A merge into main runs ``03. Release Please``, which collects release-worthy
   squash commits into a generated release PR. ``fix:`` and ``perf:`` request a
   patch, ``feat:`` requests a minor, and ``feat!:`` or a ``BREAKING CHANGE:``
   footer requests a major, including below 1.0. CI, docs, tests, and chores alone
   do not request releases.
2. Release Please updates ``pyproject.toml``, ``uv.lock``, its manifest, and
   ``CHANGELOG.md``. Other open feature PRs are excluded until merged. Review
   the proposed version and wait for the release PR's CI to pass.
3. Merge the release PR only when ready to publish. Leaving it open defers the
   release while ordinary contributor PRs can continue merging. Its squash merge
   creates the tag and GitHub release and dispatches ``04. Publish`` from main.
4. The publisher runs CI against the immutable tag, verifies tag/version and
   main ancestry, and uses those CI distribution artifacts. Project-specific
   checks below run before uploading to PyPI.
5. After a successful PyPI upload, trigger released docs on Read the Docs. A
   TestPyPI dispatch uses the same publication flow but does not trigger release
   docs. Inspect the final Read the Docs build as well as the Actions result.

Publication recovery
--------------------

PyPI versions and tags are immutable. Inspect uploaded files before retrying a
partial publication. Run ``04. Publish`` manually from main with the existing
tag and destination to use workflow repairs without changing released source.
No new version or tag is needed for a workflow-only repair. This manual dispatch
publishes a package; use it only when publication is intended. Merging a CI or
documentation contributor PR alone does not authorize or initiate a release.

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

Shared publication configuration
--------------------------------

Both projects dispatch publication from main, reuse the release CI distribution
artifacts, generate PEP 740 attestations, and upload with ``uv publish`` using
trusted publishing. Released documentation is synchronized and activated on
Read the Docs after a successful upload. Dependency Graph is generated by
GitHub repository settings; it is shared security infrastructure, not a fifth
repository workflow file.

CI validation remains specific to each package: pysystemc requires Python 3.14,
native manylinux repair, and installed-wheel simulation checks; pyipxact-de
requires its XML tooling and schema/application tests.
