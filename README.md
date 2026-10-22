# DiffImg for Windows

An independent Windows x64 revival of xbee's DiffImg, building on the lingo, sandsmark and arbruijn community forks. Baseline: arbruijn/diffimg `7c5715f89fb152c0d12a76a3e46a86122d7641bd`.

This milestone provides a portable GUI and batch companion, fixes observed image-comparison bugs, and adds reproducible build instructions and regression tests. It preserves the original Qt interface and upstream notices.

- [Windows usage, comparison semantics and rebuild instructions](README-WINDOWS.md)
- [Validation scope and limitations](VALIDATION.md)
- [Component licensing and corresponding source](THIRD-PARTY.md)
- [Pinned dependencies and download hashes](dependencies.lock.json)
- [Original upstream README](README.txt)

The runtime uses the old Qt 5.15.2 baseline. This is a tested revival milestone, not a security-audited release or Qt 6 port. Download and extract the entire portable package; dependency source bundles are provided separately with the release.
