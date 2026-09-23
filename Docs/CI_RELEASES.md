# CI and binary releases

## Workflows

- **CI**: Python syntax, regression fixtures and PowerShell parsing on a GitHub-hosted Ubuntu runner, on pushes and pull requests. No Unreal installation, Git LFS downloads or credentials are needed. It is also reused by the release workflow on the selected source commit.
- **Unreal validation**: manual execution on `main` on a configured GitHub-hosted Windows runner, building the editor and running the existing `Recovery.*` physics automation. Reports are retained as Actions artifacts. This is an actual engine test, separate from the hosted tool tests.
- **Release**: a `v0.1.0-alpha.N` tag, or a manual run from `main` with an existing tag. The tag must be on `main` history and match `ProjectVersion` in `Config/DefaultGame.ini`. It must contain the release tooling. Build, native model tests, cook, ZIP verification and publication are sequential gates. GPU acceptance is performed locally and is not a publication gate. CI archives and release.json explicitly record `gpu_validation: not_run`.

## One-time GitHub configuration

Set the **repository variable** `RELEASES_ENVIRONMENT` to the exact name of the GitHub environment holding the release secrets (for example `releases` or `PROD`). This selector must be repository-scoped; it cannot live only inside the environment it selects. Set repository variable `UE_WINDOWS_RUNNER` to the name of the GitHub-hosted Windows x64 runner using the prepared Unreal image, and `UE_ROOT` to its Unreal installation directory. Both workflows fail early when these variables are missing; there is no self-hosted fallback.

In that environment configure:

| Type | Name | Value |
| --- | --- | --- |
| Secret | `RELEASES_S3_ACCESS_KEY_ID` | Upload account key |
| Secret | `RELEASES_S3_SECRET_ACCESS_KEY` | Upload account secret |
| Variable | `RELEASES_S3_ENDPOINT` | `https://cdn.mathislambert.fr` |
| Variable | `RELEASES_S3_BUCKET` | `software-releases` |
| Variable | `RELEASES_S3_REGION` | `us-east-1` |
| Variable | `RELEASES_S3_FORCE_PATH_STYLE` | `true` |
| Variable | `RELEASES_PUBLIC_BASE_URL` | `https://cdn.mathislambert.fr/software-releases` |

The environment may require release approval. Allow the release tags and `main` for manual retries in its deployment rules. Credentials are exposed only to the publication step on the hosted Ubuntu runner. The local MinIO `.env` is neither read nor committed.

## GitHub-hosted Unreal build machine

Use a GitHub-hosted Windows x64 larger runner with a private, version-pinned custom
image containing Unreal Engine 5.8, MSVC/Windows SDK, Git LFS and PowerShell 7.
Include the engine editor plugins enabled in `SuperHeavySim.uproject`
(ModelContextProtocol and AllToolsets). Native model tests use `-nullrhi`; building
and cooking do not require a DLSS GPU or an interactive graphics session.

Custom images/larger Windows runners require a GitHub Team or Enterprise Cloud
organization. A personal repository cannot use this setup simply by changing a
YAML label. Provision the organization/runner access first. The standard Windows
image has no Unreal installation; this workflow intentionally does not pretend
`windows-latest` alone can build the project.

Create the private image using GitHub's image-generation runner process and a
licensed Unreal Installed Build. Pin the image version in the runner settings.
Size storage for the engine, LFS checkout, intermediate/cooked data and archive;
measure actual footprint before selecting a size. Keep the engine out of public
S3 release storage, workflow artifacts and shared public caches. Image provisioning
is an administrator operation, not implemented by these project workflows.
Never bake credentials into the image. Scope runner access to trusted repositories;
Unreal execution is main/tag-only, not a pull-request job.

No paid runner, image or organization is created automatically by this repository.
Without the configured machine, only the existing hosted tooling CI can run.
macOS compilation/packaging is not implemented by this Windows workflow.

References: [GitHub custom images](https://docs.github.com/en/actions/how-tos/manage-runners/larger-runners/use-custom-images)
and [Epic Installed Builds](https://dev.epicgames.com/documentation/unreal-engine/installed-build-reference-guide-for-unreal-engine).

## Local graphics acceptance

Download/extract the published ZIP into `Releases/` on a Windows NVIDIA/DLSS machine.
Check out the source commit in its `build-manifest.json`, install Python 3.12 on PATH
and the packaged Windows prerequisites, then run:

```powershell
./Tests/Unreal/Packaging/test_windows_package.ps1 -Version 0.1.0-alpha.12
```

The runner verifies package hashes and the checkout identity before executing
startup, controls, physical capture, camera, cloud and audio checks. It does not
require Unreal. Inspect the fresh screenshots as well. Local reports stay in
`Saved/Recovery`; they do not retroactively change an immutable published release.

## Publish a version

1. Update `ProjectVersion` and the release documentation, commit and merge to `main`.
2. Create and push an annotated matching tag, e.g. `git tag -a v0.1.0-alpha.12 -m "Alpha 12"` then `git push origin v0.1.0-alpha.12`.
3. Inspect the native-test and package-integrity evidence. Run graphics acceptance locally against the downloaded package; CI does not establish visual quality.
4. The Release summary contains download links after successful publication.

The AWS CLI is pinned to **2.36.50**, installed from its versioned official distribution. The uploader uses path-style addressing when configured, HTTPS, explicit file uploads and SHA-256 metadata. It never synchronizes unrelated directories, deletes bucket contents or uploads original sources.

Objects live under `<repository>/<tag>/<filename>`:

```text
SpaceX-Superheavy-v3-Simulator/v0.1.0-alpha.12/
  Starbase-0.1.0-alpha.12.zip
  Starbase-0.1.0-alpha.12.zip.sha256
  Starbase-0.1.0-alpha.12-artifacts.json
  release.json
```

`release.json` is uploaded last, after remote size/checksum-metadata verification of each artifact. Consumers should use it as the completion marker. Identical retries resume partial uploads; existing objects with conflicting hashes or unexpected files cause a failure. A published tag is immutable: use a new version to change a binary. Configure a bucket lifecycle rule to abort abandoned multipart uploads after a suitable retention period.

The public URLs are direct object URLs; anonymous bucket listing is not required. Consumers can independently verify the ZIP against its `.sha256` file. Publication tests use an in-memory store and cover interrupted transfers, retries, conflicting releases, provenance checks and incomplete completion markers. A real credentialed upload still requires the configured GitHub environment and runner.
