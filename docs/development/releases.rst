Automated pull requests and releases
====================================

Contributor workflow
--------------------

Work on a feature branch and open a pull request. CI tests the complete suite,
produces coverage for handwritten application code, builds distributions, and
builds documentation. Codecov reports coverage; Read the Docs builds PR previews
through its GitHub App. No pull request publishes a production package.

Squash-merge contributor PRs using a Conventional Commit title. Require the
PR title, Tests and coverage, Build distributions, and Build documentation
checks in branch protection. Keep at least one review requirement and require
branches to be current, or use a merge queue. Do not enable automatic merging
of arbitrary contributor PRs.

Versions and concurrent contributors
------------------------------------

Contributors do not edit package versions or create release tags. Release-please
collects merged Conventional Commits into one release PR and updates
pyproject.toml, the release manifest, uv.lock, and CHANGELOG.md. It refreshes
that PR when more changes merge. The highest bump among merged changes wins.
Open PRs are excluded until merged. Thus multiple contributors never reserve or
race to increment versions in their individual branches.

* ``fix: ...`` and ``perf: ...`` request a patch (0.12.0 to 0.12.1).
* ``feat: ...`` requests a minor (0.12.0 to 0.13.0).
* ``feat!: ...`` or a ``BREAKING CHANGE:`` footer requests a major (0.12.0 to 1.0.0).
* Documentation, tests, CI and chores alone do not request a release.

These rules also apply below 1.0 because the pre-major special bump behavior is
explicitly disabled. Maintainers can override a proposed version with the
release-please ``Release-As:`` mechanism after reviewing the intended API impact.
Tags record the chosen version; they do not calculate it. Merging the reviewed
release PR creates its GitHub release/tag. Publishing then verifies the tag,
reruns all tests, builds and checks distributions, and repairs native wheels with auditwheel. Installed-wheel checks validate
startup, callbacks, simulation time and tutorials outside the build container.
The PyPA publishing action uses PyPI trusted publishing and PEP 740 attestations;
uv manages dependencies and builds through the native preparation backend. Publication runs in the same automation chain, so
it does not rely on a bot-created tag triggering another workflow.

The initial manifest starts at 0.12.0, already tagged and published on PyPI.
This automation does not republish it. CI-only changes do not request a new version.

One-time service configuration
------------------------------

* Enable Actions permissions to create pull requests.
* Set ``RELEASE_PLEASE_TOKEN`` to a fine-grained personal token belonging to
  ``amal-khailtash`` with contents and pull-request write permissions. A default GITHUB_TOKEN-created PR does not trigger ordinary
  PR workflows; that is why a separate token is required. Rotate expiring tokens.
  A GitHub App is an alternative, but requires adding a step to mint a fresh
  installation token on each run; do not store a short-lived installation token.
* Enable squash merging and require Conventional Commit PR titles. Configure
  branch protection checks and reviews. The release bot must not bypass tests.
* Create the ``pypi`` GitHub environment. In PyPI's trusted publishers, authorize
  this repository, ``publish.yml``, and environment ``pypi``. No PyPI password
  or API token is needed. Required environment reviewers are optional if fully
  automatic publication after a reviewed release PR is desired.
* Install Codecov's GitHub App, register the repository, and configure
  ``CODECOV_TOKEN`` if required. Coverage is initially informational; service
  outages or fork token restrictions do not block otherwise valid PRs.
* Import this repository in Read the Docs, install its GitHub App, enable PR
  builds, and enable the GitHub webhook integration. Activate released tag
  versions using Read the Docs version automation rules. The App/webhooks build
  latest, PR previews, and tag documentation from ``.readthedocs.yaml`` for PR previews. The existing ``readthedocs.yml`` also validates documentation
  and triggers latest/released builds using ``READTHEDOCS_TOKEN``; keep that
  secret configured and avoid duplicate automatic builds for those refs.

Existing actions and retries
----------------------------

``ci.yml`` remains the CI entry point. ``publish.yml`` is now the reusable
verified publisher, called by release automation; manual dispatch takes an
existing release tag for recovery after publication fails. PyPI versions are
immutable: do not overwrite a published version or reuse a tag with new code.
Inspect which files were uploaded before retrying a partially completed upload.

Manual dispatch of ``publish.yml`` takes an existing tag and either ``pypi``
or ``testpypi`` as the destination. TestPyPI publication does not create a release.

For this repository, configure these required checks: ``PR title / title``,
``Tests and coverage``, ``Build distributions`` and ``Build documentation``.
Confirm their exact names on the first PR before saving branch protection.
A sole maintainer cannot approve their own PR; arrange another reviewer before
enforcing a required review. Release PRs must receive the same checks and review.

Create a fine-grained token at GitHub Settings > Developer settings > Personal
access tokens, limited to this repository, then install it without putting the
value in shell history::

   gh secret set RELEASE_PLEASE_TOKEN --repo amal-khailtash/pysystemc

Enable Settings > Actions > General > Allow GitHub Actions to create and approve
pull requests. Under Settings > Rules > Rulesets, protect ``main``, require PRs,
require the checks above and current branches, block force pushes/deletions, and
require one review when another reviewer is available. Under Settings > General,
enable squash merging and use the PR title for the squash commit title.

At PyPI, configure owner ``amal-khailtash``, repository ``pysystemc``, workflow
``publish.yml``, environment ``pypi``. The environment already exists; verify
its trusted-publisher entry remains correct. ``CODECOV_TOKEN`` and
``READTHEDOCS_TOKEN`` already exist; ``RELEASE_PLEASE_TOKEN`` still needs setup.
Main was unprotected and Actions PR creation was disabled when this flow was
prepared. These service settings are separate from merging the workflow PR.

Implementation sources
----------------------

* `Google release-please <https://github.com/googleapis/release-please>`_
* `Release-please GitHub Action <https://github.com/googleapis/release-please-action>`_
* `uv trusted publishing example <https://github.com/astral-sh/trusted-publishing-examples>`_
* `Codecov action <https://github.com/codecov/codecov-action>`_
* `Read the Docs configuration <https://docs.readthedocs.com/platform/stable/config-file/v2.html>`_

This flow follows pyipxact-de’s release automation while retaining pysystemc’s
native manylinux/auditwheel packaging and installed-consumer validation.
Python 3.14 and pthread SystemC remain required.
