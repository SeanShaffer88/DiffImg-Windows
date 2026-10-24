# DiffImg for Windows

An independent Windows x64 revival of xbee's DiffImg, building on the lingo, sandsmark and arbruijn community forks. Baseline: arbruijn/diffimg `7c5715f89fb152c0d12a76a3e46a86122d7641bd`.

## TL;DR

DiffImg for Windows is a revived, portable Windows x64 build of the DiffImg image-comparison tool. Select two images with the same dimensions and it shows exactly which pixels differ, with per-channel, channel-mean, luminance and perceptual metrics, statistics, and a batch command-line companion for automated checks. Version 2.6.0 adds CI reports and faster screenshot review (see VALIDATION.md for exactly what was tested and what was not) and is free software under the GPL. Download the portable ZIP from the latest release, extract it into a writable folder, and run diffimg.exe - no installer, administrator access, or Qt SDK needed. The release package uses a self-signed Authenticode certificate; this does not establish public Windows publisher trust.

Version 2.6.0 provides a portable GUI and batch companion, fixes observed image-comparison bugs, and adds reproducible build instructions and regression tests. It preserves the original Qt interface and upstream notices.

- [Windows usage, comparison semantics and rebuild instructions](README-WINDOWS.md)
- [Validation scope and limitations](VALIDATION.md)
- [Component licensing and corresponding source](THIRD-PARTY.md)
- [Pinned dependencies and download hashes](dependencies.lock.json)
- [Original upstream README](README.txt)

The runtime uses Qt 6.12.0 with a C++17 build. Native comparison, GUI, CLI, parser robustness and deployment checks are documented in VALIDATION.md. Download and extract the entire portable package; dependency source bundles are provided separately with the release.

## Screenshot project workflow

2.5.0 adds folder comparisons, named profiles and ignore regions, a slider/synchronized preview with change navigation, standalone HTML CI reports, recoverable baseline approval/restore, and Windows icon/version metadata. Open **Project review** (Ctrl+Shift+R) or **Try project review.bat**. See README-WINDOWS.md for usage, backup and signing contracts.

2.6.0 adds JSON/JUnit CI reports, F8/Shift+F8 failing-file navigation, all/visible/selected HTML report scope and normalized RGB pixel inspection.
