# CI and binary releases

## Workflows

- **CI**: Python syntax, regression fixtures and PowerShell parsing on a GitHub-hosted Ubuntu runner, on pushes and pull requests. No Unreal installation, Git LFS downloads or credentials are needed. It is also reused by the release workflow on the selected source commit.
- **Unreal validation**: manual execution on `main`, building the editor and running the existing `Recovery.*` physics automation. Reports are retained as Actions artifacts. This is an actual engine test, separate from the hosted tool tests.
- **Release**: a `v0.1.0-alpha.N` tag, or a manual run from `main` with an existing tag. The tag must be on `main` history and match `ProjectVersion` in `Config/DefaultGame.ini`. It must contain the release tooling. Build, cook, packaged startup/controls/flight/audio/capture audits, ZIP verification and publication are sequential gates. No failed or untested package is published.

## One-time GitHub configuration

Set the **repository variable** `RELEASES_ENVIRONMENT` to the exact name of the GitHub environment holding the release secrets (for example `releases` or `PROD`). This selector must be repository-scoped; it cannot live only inside the environment it selects. Set repository variable `UE_ROOT` to the Unreal installation on the build runner.

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

## Unreal runner

Register a dedicated Windows x64 self-hosted runner with label **`unreal-5.8-nvidia`**. Install Unreal Engine **5.8**, its C++ build prerequisites (Visual Studio/MSVC and Windows SDK), Git LFS, PowerShell 7 and a DLSS-capable NVIDIA GPU/driver. The existing packaged audit explicitly verifies DLSS; a headless/basic hosted runner is not sufficient. Run the agent in an interactive desktop session for the existing graphics/audio tests, not an isolated Windows service session. Provision the editor plugins enabled by `SuperHeavySim.uproject`, including ModelContextProtocol and AllToolsets, or use the same engine installation as local development.

Use a dedicated runner checkout with adequate disk space for the engine, LFS assets, cooked content and ZIP. `actions/checkout` cleans that workspace between builds; never point the runner checkout at a personal working copy. Builds are serialized, and no pull-request workflow runs code on this machine. Scope the runner to this repository. Do not store S3 credentials on it.

The engine is not distributed or downloaded by Actions. Registering the machine and installing Unreal are prerequisites, not simulated CI steps. macOS is deliberately absent until its native package and validation exist.

## Publish a version

1. Update `ProjectVersion` and the release documentation, commit and merge to `main`.
2. Create and push an annotated matching tag, e.g. `git tag -a v0.1.0-alpha.12 -m "Alpha 12"` then `git push origin v0.1.0-alpha.12`.
3. Inspect the physical and packaged audit evidence. The tests do not replace human review of rendered screenshots.
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
