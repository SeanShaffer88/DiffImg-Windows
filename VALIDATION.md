# Windows x64 release validation — 2026-10-23

## Tested local scope

Qt 6.12.0 official MSVC2022 x64 shared SDK, Qwt 6.3.0 pinned source, MSVC 19.44.35229 / toolset 14.44.35207, Windows SDK 10.0.26100.0, CMake 3.31.6 and Ninja 1.11.1.4. Dependencies and archive digests are in dependencies.lock.json.

- 22 QtTest cases pass (24 recorded passes including setup/cleanup): independent numerical expectations, 40 seeded random pixel oracles, odd-width grayscale, mixed formats, premultiplied/64-bit conversion, alpha, mean/luminance semantics, dimension rejection, stale-result clearing, missing/invalid inputs, immutable inputs, perceptual identity/differences, one-pixel-wide perceptual inputs, pyramid bounds, parameter boundaries, size limits, Qt 6 property types/regex, codec round trips and native GUI behavior.
- 31 independent CLI checks pass, including stdout/exit codes, threshold boundaries, invalid arguments/settings, explicit refusal of unsupported downsampling, safe output behavior and Unicode inputs.
- 126 seeded malformed/mutated PNG/BMP/JPEG/GIF/ICO cases complete with ordinary exit codes and immutable input bytes. Truncated and structurally invalid cases must return error 2. Mutations may decode or be rejected. Each process has a timeout. This is a bounded robustness corpus, not exhaustive fuzzing.
- Native GUI smoke tests pass at scale factors 1, 1.25, 1.5 and 2. Screenshots were inspected. Navigator minimum height and statistics columns were corrected. Tests exercise original/modified/difference views, split panels, wipe, fit, actual size and refresh.
- Portable tests verify the SHA256 manifest and AMD64 PE headers, relocate to space-containing and Unicode paths, remove Qt/QML environment and restrict PATH to System32, run the full QtTest and CLI suites, and launch the GUI. Loaded Qt, Qwt, platform and VC runtime modules must match package files by file identity. Supplemental QtTest runtime is used only in the temporary QA folder.
- Source archive digest matches the upstream Qtbase checksum. Qt 6 license texts and bundled third-party notices accompany the portable folder; complete corresponding source archives accompany release assets.

## Project review — 2.5.0 validation

The 22 existing native cases remain regression gates. Thirteen additional native project-review cases cover folder/status matching, thresholds and double-precision boundaries, overlapping/global/per-file masks across metrics, profile round trips/rejections, decoder failures, escaped/thin-image reports and stale-data refusal, approval/addition/restoration, failed preparation, prepared recovery, corrupt backups, stale conflicts, Windows junction refusal and the native GUI workflow. Twenty independent folder CLI cases cover exit codes, profiles, masks, reports, malformed inputs and absence of automatic approval. Workbench smoke tests run at 100/125/150/200% scaling; screenshots are inspected.

Stable publication requires native/CLI/parser checks, AddressSanitizer on application/bundled code, relocated space/Unicode portable tests including the workbench, and exact-downloadable-ZIP deployment on Windows Server 2022/2025. Release notes and release-verification JSON record the final commit, workflow and asset digests. Corresponding application/Qtbase/Qwt sources and notices accompany the binary.

Unsigned application EXEs are compared across independent build directories before signing. Qwt DLLs remain path-dependent; whole-package/cross-toolchain byte identity is not claimed. Authenticode signatures differ from unsigned reproducible-build bytes. Icon/version metadata and signature identity are checked separately. Self-signed signing is not public Windows trust; no private keys or trust-store changes are packaged.

## Practical limits

Vendor Qt/Qwt DLLs are not sanitizer-instrumented. Hosted Windows Server images and local Windows desktop build 26300 are exercised environments, not every consumer configuration or pristine consumer VM. x86/ARM64, HDR/profile equivalence, animated frames, exhaustive codec fuzzing, accessibility certification and all perceptual settings remain outside scope. Qt 6.12.0/MSVC x64 and the equal-dimension, 8-bit RGB-over-white contract remain. Reports use scaled previews; region navigation uses approximate 32px tiles; masks use fixed pixel coordinates.

Baseline operations use hash checks, an operation lock, verified originals and per-file atomic replacement/recovery. They are not filesystem-wide atomic transactions or a concurrency guarantee against unrelated editors. Prepared recovery, junctions and conflict/error cases are tested; power-loss/storage-controller behavior is not certified. Backups require space and remain until the user removes them. See README-WINDOWS.md for exact recovery and signing behavior.
