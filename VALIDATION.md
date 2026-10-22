# Windows x64 release validation — 2026-10-22

## Tested local scope

Qt 6.12.0 official MSVC2022 x64 shared SDK, Qwt 6.3.0 pinned source, MSVC 19.44.35229 / toolset 14.44.35207, Windows SDK 10.0.26100.0, CMake 3.31.6 and Ninja 1.11.1.4. Dependencies and archive digests are in dependencies.lock.json.

- 22 QtTest cases pass (24 recorded passes including setup/cleanup): independent numerical expectations, 40 seeded random pixel oracles, odd-width grayscale, mixed formats, premultiplied/64-bit conversion, alpha, mean/luminance semantics, dimension rejection, stale-result clearing, missing/invalid inputs, immutable inputs, perceptual identity/differences, one-pixel-wide perceptual inputs, pyramid bounds, parameter boundaries, size limits, Qt 6 property types/regex, codec round trips and native GUI behavior.
- 31 independent CLI checks pass, including stdout/exit codes, threshold boundaries, invalid arguments/settings, explicit refusal of unsupported downsampling, safe output behavior and Unicode inputs.
- 126 seeded malformed/mutated PNG/BMP/JPEG/GIF/ICO cases complete with ordinary exit codes and immutable input bytes. Truncated and structurally invalid cases must return error 2. Mutations may decode or be rejected. Each process has a timeout. This is a bounded robustness corpus, not exhaustive fuzzing.
- Native GUI smoke tests pass at scale factors 1, 1.25, 1.5 and 2. Screenshots were inspected. Navigator minimum height and statistics columns were corrected. Tests exercise original/modified/difference views, split panels, wipe, fit, actual size and refresh.
- Portable tests verify the SHA256 manifest and AMD64 PE headers, relocate to space-containing and Unicode paths, remove Qt/QML environment and restrict PATH to System32, run the full QtTest and CLI suites, and launch the GUI. Loaded Qt, Qwt, platform and VC runtime modules must match package files by file identity. Supplemental QtTest runtime is used only in the temporary QA folder.
- Source archive digest matches the upstream Qtbase checksum. Qt 6 license texts and bundled third-party notices accompany the portable folder; complete corresponding source archives accompany release assets.

## Independent rebuild

Two independent build directories produced identical application bytes:

| File | SHA256 |
|---|---|
| diffimg.exe | 5d54637e893efeec788522702bedcaf3afff92c7aed71384303953f2497dbece |
| diffimg-cli.exe | 9c9089b49c1d045ecca895140a320290137db88540c8ab5ec05b46ee5fc5efb1 |

Qwt DLLs rebuild and pass tests but differ with build path (observed MSVC anonymous-namespace type names). The release pins its actual DLL digest. Whole-package, cross-machine and cross-toolchain byte identity is not claimed.

## Stable publication gates

The Windows workflow must build the release source, run native/CLI/robustness checks, and pass AddressSanitizer checks for DiffImg and bundled comparison/property-browser code. Vendor Qt/Qwt DLLs are not sanitizer-instrumented. This local MSVC installation lacks the x64 sanitizer runtime; sanitizer execution is performed on the hosted build runner. The workflow also verifies a rebuilt portable package on separate Windows Server 2022 and 2025 runners without installing a Qt SDK in the deployment jobs.

Before stable promotion, the exact downloadable portable ZIP is independently SHA256-checked and tested on both hosted Windows images. Release notes link the exact source commit, successful run and asset digests. A prerelease candidate is not promoted while these gates fail or remain pending.

## Practical limits

Hosted runners are fresh separate Windows environments, not pristine consumer Windows VMs. Local native desktop validation and hosted Server tests do not establish every Windows 10/11 configuration. Intended platform: Windows 10 1809+ / Windows 11 x64, consistent with Qt 6.12's supported configurations; Windows desktop build 26300 and the hosted Server images are the exercised environments. x86, ARM64, other OSes, HDR/profile equivalence, all perceptual parameter combinations, animated GIF frames and comprehensive adversarial codec testing remain outside scope. The application is unsigned. The package needs a writable folder for adjacent settings.

The comparison contract is equal dimensions and 8-bit RGB composited over white. Downsample must be 0; legacy nonzero values were silently ignored. Qt 6 porting does not establish codec security. Continue tracking toolkit updates. Original upstream notices and harmless legacy diagnostics remain; this is not a claim that the entire codebase is warning-free or comprehensively reviewed.
