Pull requests and releases
==========================

Workflow order
--------------

The workflows separate code validation from package publication:

* **01. PR title** checks Conventional Commit titles using metadata only.
* **02. CI** runs full tests, coverage, distribution checks and documentation
  checks on feature PRs. Release Please PRs receive focused release-metadata
  validation instead. Merging into main does not repeat full CI.
* **02a. Build distributions (reusable)** packages the selected revision for CI
  or publication.
* **02b. Tests and coverage (reusable)** validates feature PRs and reports
  tagged release coverage after PyPI verification, with four test workers.
* **03. Release Please** prepares a release PR and synchronizes its lockfile.
  Merging the release PR creates an immutable tag and a **draft** GitHub release,
  then dispatches publication from main.
* **04. Publish** builds the tagged distributions once, checks the installed
  wheel, uploads those same files and verifies their availability and SHA-256
  hashes on PyPI. Only after that verification does it upload coverage for the
  tagged source revision to Codecov's ``main`` branch, publish the GitHub release
  and activate released documentation.

``distributions.yml`` is a reusable build job called by feature CI and the
publisher. It has no independent trigger. A feature packaging check validates
that changes can be packaged; publication builds again because the final release
version differs from the feature PR. The complete source test suite is not
repeated during publication.

Read the Docs PR previews and development ``latest`` documentation remain
independent of package publication. CI builds documentation to detect errors;
that build does not activate a released version. Disable automatic tag builds
and automatic activation of new release versions in Read the Docs if strict
PyPI-before-release-documentation ordering is required. Keep tag versions
available for discovery; the publisher activates them after verification.

Contributor branches
--------------------

Create branches from current main using ``<type>/<short-description>``::

   git switch main
   git pull --ff-only
   git switch -c feat/custom-interfaces

Use ``feat``, ``fix``, ``ci``, ``docs``, ``test``, ``build``, ``perf`` or
``refactor`` according to the change. The squash commit's PR title determines
release impact. CI, documentation, tests and chores alone do not request a
release. Contributors do not create manual version branches or release tags.

Feature PR validation
---------------------

1. Implement and validate on the contributor branch, update relevant docs and
   changelog notes, commit, push and open a PR against main.
2. Full CI calls the reusable coverage workflow, which prepares native SystemC,
   runs lint and formatting checks, and runs tests and subprocess coverage in
   four pytest workers using work stealing. It reports the 15 slowest tests.
   Every simulation still uses a fresh native process.
3. Review the title, validation selection, tests, distributions and documentation
   results before merging. Configure required checks to match this workflow;
   skipped full jobs on a validated release-metadata PR are intentional.
4. Squash-merge using the Conventional Commit PR title. Update local main and
   delete the merged contributor branch. Squash history can require checking
   the merged PR before deleting a branch with ``git branch -D``.

Use ``make test-parallel TEST_WORKERS=4`` or
``make coverage-parallel TEST_WORKERS=4`` locally. The Makefile defaults remain
at two workers for machines with fewer resources. CI can also be dispatched
manually for full validation without a PR.

Release validation and publication
----------------------------------

1. Release Please collects release-worthy merged changes. ``fix`` and ``perf``
   request a patch, ``feat`` requests a minor, and breaking changes request a
   major, including below 1.0.
2. The generated PR updates the project version, manifest and changelog. A
   follow-up automation runs ``uv lock``, moves pending ``Unreleased`` changelog
   details into the new release section, and commits changed metadata to the same
   PR. It leaves exactly one empty ``Unreleased`` section at the top. This may
   restart the short metadata check. Write user-visible changes under that one
   pending section while developing; release summaries and detailed notes remain
   together after publication.
3. The metadata check compares the actual Git trees. Only ``pyproject.toml``,
   ``uv.lock``, the release manifest and ``CHANGELOG.md`` may change. Project
   settings and dependency records must remain identical apart from the local
   package version. Versions must agree and the changelog must contain the new
   version. Unexpected changes fail validation instead of bypassing source CI.
4. Review and merge the release PR when ready to publish. Its tag identifies the
   source exactly; the GitHub release remains a draft during publication.
5. The publisher builds and repairs native manylinux wheels, checks distribution
   metadata, verifies the tag/version and main ancestry, and tests the installed
   wheel outside the build container. Native resources, runtime compilation,
   pthread waits, simulation time and bundled examples are checked before upload.
6. Trusted publishing uploads the saved artifacts to PyPI. The index must expose
   every wheel and source distribution with matching SHA-256 hashes and no
   yanked file. Bounded retries allow for index propagation.
7. After verified publication, the reusable coverage workflow tests the tagged
   source revision and explicitly reports its resolved commit SHA on Codecov's
   ``main`` branch. Other jobs attach the distributions and publish the GitHub
   release, and activate/build the version on Read the Docs. A TestPyPI dispatch
   verifies its uploaded artifacts but does not upload released coverage,
   announce a GitHub release or activate release documentation.

Failure and retry behavior
--------------------------

A build, installed-wheel or upload/availability failure leaves the GitHub release
as a draft and does not activate released documentation or update released
coverage. Coverage results from feature validation remain available; they
describe tested code, not publication. If the released-coverage job fails after
PyPI verification, rerun that job to report the same tagged source revision.

If PyPI succeeds but GitHub or documentation fails, the package is already
published. Re-run the failed announcement job without uploading again. The docs
job queues a Read the Docs build; inspect that service for the final build result.

Artifacts are retained for 30 days. Prefer re-running failed jobs in the original
publication run so recovery uses the same distribution files. PyPI versions and
release tags are immutable. Inspect a partial upload before retrying; verification
rejects files whose index hashes differ from the saved artifacts.

For a workflow repair or expired artifacts, manually dispatch ``04. Publish``
from main with the existing tag and ``pypi`` or ``testpypi`` destination. This
rebuilds from the tag and verifies uploaded bytes. It cannot silently replace an
existing PyPI file with different bytes. Publication verification tooling comes
from the workflow revision, allowing fixes without changing released source.

Service configuration
---------------------

* Enable Actions PR creation and configure ``RELEASE_PLEASE_TOKEN`` with contents
  and pull-request write access. Generated PR and lockfile commits must trigger
  metadata checks; draft release publication uses this token too.
* Configure PyPI and TestPyPI trusted publishers for ``publish.yml`` and its
  respective ``pypi`` / ``testpypi`` environments. These identities are unchanged.
* Configure ``CODECOV_TOKEN`` for feature-PR and post-PyPI released coverage,
  and the Read the Docs GitHub integration for PR previews and development docs.
* Configure ``READTHEDOCS_TOKEN`` with released-version activation and build
  permissions, and disable independent automatic release-version activation.

These workflows describe this repository. Related projects may use different
release triggers and must be updated independently.
