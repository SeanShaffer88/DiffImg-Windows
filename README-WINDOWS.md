# DiffImg for Windows — 2.4.0

Windows x64 revival release based on arbruijn/diffimg commit `7c5715f89fb152c0d12a76a3e46a86122d7641bd`, descended from the sandsmark and lingo forks and xbee's original DiffImg. Original authorship and license notices are preserved. This is an independent revival, not an official upstream release.

## Portable use

Extract the entire portable ZIP into a writable folder, then run **Start DiffImg.bat** or **diffimg.exe**. Keep the executable, DLLs, plugin folders and qt.conf together. No Python, Qt SDK, installer or administrator access is needed to run the package. Windows 10/11 x64 is the intended target; local tests were performed on the development host, not every Windows version or a clean VM.

Settings are written to `DiffImg.ini` beside the executable. This folder must be writable. The command-line companion is `diffimg-cli.exe`; use it for captured stdout and exit codes. The GUI retains the original controls and keyboard shortcuts. Select two images, choose a metric in Preferences, and inspect the difference mask and statistics.

## Comparison contract

- Both images must have exactly the same dimensions. Different dimensions are an error; this revival removed silent resampling/padding that could hide regressions.
- Inputs are normalized to 8-bit RGB, compositing alpha over a white background. Fully transparent hidden RGB is ignored; alpha changes that alter the white-background appearance count as differences. This is an explicit policy, not raw RGBA identity or color-managed perceptual equivalence.
- Per-channel differences use absolute RGB channel differences. Statistics use all RGB channel samples. A pixel differs when any channel is nonzero.
- Channel-mean mode uses the absolute difference of RGB sums divided by 3 (integer truncation). Equal RGB means can compare equal even if colors differ; use per-channel mode for strict screenshot checks.
- Luminance mode uses integer-rounded `0.30R + 0.59G + 0.11B`, matching the displayed description rather than the legacy HSL-lightness implementation.
- Perceptual mode is the bundled legacy perceptual-diff algorithm after the same alpha normalization. It is not a modern perceptual-accuracy guarantee. Its parameters require separate calibration for your images. The legacy Downsample setting was ignored by the application; only 0 is now accepted, and nonzero values return an error.
- Headers are checked before decoding and each image is limited to 16,777,216 pixels. This does not make the image codecs a sandbox. PNG, BMP, JPEG, GIF and ICO are the packaged formats; animated GIF frame comparison, TIFF/WebP/JP2 and HDR/color-profile equivalence are outside the tested scope.
- Inputs are read-only during comparison. Batch output must be a new file; an existing destination is refused, including an input path. The GUI's normal save dialog remains an explicit user action.

## Batch examples

```powershell
.\diffimg-cli.exe --batch baseline.png current.png
.\diffimg-cli.exe --batch --threshold ErrorNum=10 --threshold MaxError=5 baseline.png current.png
.\diffimg-cli.exe --batch --output new-difference.png baseline.png current.png
.\diffimg-cli.exe --list
```

Exit codes: **0** accepted, **1** differences exceed the selected thresholds, **2** invalid input/options or output failure. With no threshold supplied, the default is exact pixel equality (`ErrorNum=0`). All specified thresholds must pass. Identical images also produce a zero-difference image when `--output` is supplied.

## Rebuild

Use Python 3.11+ for the build scripts, Git, MSVC x64 tools and Windows SDK 10.0.26100.0. Tested compiler: MSVC 19.44.35229 (toolset directory 14.44.35207). Runtime DLLs came from the matching VS2022 x64 redistributable directory 14.44.35112. No global environment modifications are required.

Provision isolated build tools and the exact Qt archives:

```powershell
python -m venv .build-tools
.\.build-tools\Scripts\python.exe -m pip install aqtinstall==3.3.0 cmake==3.31.6 ninja==1.11.1.4
.\.build-tools\Scripts\python.exe tools/fetch_qt.py --output .deps/Qt
git clone https://git.code.sf.net/p/qwt/git .deps/qwt
git -C .deps/qwt checkout 4ddf2f3878d0835f678da0f2aba1180f2ba208f6
python tools/build_windows.py --qt-root .deps/Qt/6.12.0/msvc2022_64 --qwt-source .deps/qwt --msvc-root "YOUR-VS/VC/Tools/MSVC/14.44.35207" --tools-dir .build-tools/Scripts --build-root build-windows
```

The script copies Qwt into the build folder, disables unused SVG/OpenGL/designer/examples/playground/tests features, builds Qwt and DiffImg, then runs numerical/GUI and CLI tests. It sets the compiler/SDK environment only in child processes. Qwt's source checkout must be clean at the pinned commit. The application builds as C++17 against shared Qt 6.12.0 and Qwt 6.3.0. `/Brepro` and timestamp-free Qt resource format 1 reduce nondeterministic build inputs; see the validation report for the exact rebuild comparison, rather than assuming cross-machine bit identity.

Build logs and test results are kept under the build folder. Dependencies and toolchain provenance are in `dependencies.lock.json`. Source package excludes SDK binaries. To package, obtain the matching Qtbase source archive and run `tools/package_windows.py --help`. The script requires a fresh output folder and matching app-local Microsoft CRT files. Publish the Qtbase/Qwt corresponding-source archives alongside the application source and binaries.

## Scope and maintenance

This release fixes observed pixel access, mask, metric, batch validation and Windows packaging problems. It does not claim comprehensive fuzzing, codec security certification, cross-platform support, x86/ARM64 support or exhaustive parser security. The Qt runtime is updated to 6.12.0; continue updating it as security fixes become available. No automatic update or network service is added.

Licensing and source access are described in THIRD-PARTY.md. Upstream references: https://sourceforge.net/projects/diffimg/ and https://github.com/arbruijn/diffimg .
