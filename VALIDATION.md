# Windows x64 Qt 6 release validation — 2026-10-22

Release gates are in progress. This development snapshot is not a published stable release.

Local Qt 6.12.0 / MSVC x64 build: 20 QtTest cases, 30 CLI checks and 126 deterministic malformed/mutated image cases passed. Relocated portable checks passed from space-containing and Unicode paths with Qt environment removed and PATH restricted to System32. Native GUI smoke tests passed at scale factors 1, 1.25, 1.5 and 2, and the fractional-scale screenshot was inspected. The release report will record sanitizer, independent rebuild and hosted Windows CI evidence before publication.

The comparison contract remains 8-bit RGB over white and equal dimensions. Full fuzzing, HDR/color-profile equivalence, animated frame sequences, x86/ARM64 and all Windows versions are outside scope. Hosted Windows CI is separate-machine evidence, not a pristine consumer VM or a security certification.
