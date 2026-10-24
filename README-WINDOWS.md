# DiffImg for Windows — 2.5.0

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

## Project review — 2.5.0

Open **Project review** (Ctrl+Shift+R), run **Try project review.bat**, or launch `diffimg.exe --project-review BASELINE CURRENT`. Recursive folder comparisons match supported image files by relative name (case-insensitive on Windows). Sort/filter Identical, Accepted, Changed, Missing baseline, Missing current and Error results. Limits: 20,000 images per folder, 256 MiB per file, 16,777,216 decoded pixels. Input folders must be separate and not contain one another; links/junctions are refused and .diffimg-backups is excluded.

Preview modes: draggable before/after divider, side-by-side with shared pan/zoom, difference and 50% overlay. Wheel/+/- zoom, arrows pan, F fits and 1 selects actual size. The divider slider is keyboard accessible. F7/Shift+F7 navigate connected groups of changed 32px tiles; region boxes are approximate. Controls scroll on smaller screens while Close/Cancel remain accessible.

Enable **Draw ignore region**, then drag over a preview. Amber rectangles apply to that relative filename in original pixel coordinates; Clear image regions removes them. Edit profile can add/clear global rectangles. Masks are clipped and filled white in both inputs before every metric. The workbench mean excludes the union of ignored pixels (overlaps counted once); completely masked images compare equal. Recheck masks when image dimensions change.

Save/load named JSON profiles containing metric, thresholds, perceptual settings and global/per-file regions. They store relative names, not folder paths. All thresholds must pass. Criteria edits invalidate results: compare again before reporting/approval. Workbench criteria do not overwrite the original viewer's saved metric preferences.

**HTML reports:** choose a new filename. Text is escaped; PNG previews are embedded and scaled to at most 1600px. No script/network resources are required. Existing outputs and stale inputs/criteria are refused; reports are bounded to 100 MB of generated HTML. Missing/error entries are included.

```powershell
.\diffimg-cli.exe --folder-batch --baseline .\baseline --current .\current --profile .\screenshots.json --report .\new-report.html
```

Folder exit codes: 0 all pass, 1 changed/missing images, 2 invalid inputs/profiles, decoding/report errors or empty scans. Save CLI reports outside input folders. The CLI never approves or deletes baselines.

**Approval/recovery:** select rows and choose Approve selected; confirmation lists affected files. Valid current images replace/add baselines; missing-current files never delete baselines. Hashes must still match the scan. Original bytes and a manifest are saved under `.diffimg-backups/TRANSACTION` before per-file atomic replacements. A lock prevents overlapping DiffImg approval/restore operations. Ordinary write failures attempt rollback. Preparation failures leave inputs unchanged but may leave an unused backup directory.

Restore backup accepts that baseline folder's transaction manifest, verifies originals and refuses conflicting subsequent edits. Newly approved additions are removed on restore. Prepared manifests permit recovery from interrupted approval; already-restored original bytes are skipped on retry. Keep backups until satisfied. This is per-file recovery, not a filesystem-wide transaction or a lock against other editors; storage/controller power-loss behavior is not certified. Do not edit manifests/backups manually.

**Windows polish/signing:** both executables embed the original DiffImg icon and 2.5.0 product/version metadata. The published package uses the existing SeanShaffer88 self-signed Authenticode certificate. This provides an embedded identity/integrity check, not public Windows publisher trust or SmartScreen reputation. Public certificate/SIGNING.txt accompany signed packages; no private key or trust-store changes are included. CI rebuilds remain unsigned. Packaging can use an existing certificate via `--sign-thumbprint THUMBPRINT`.

## Review productivity additions (development)

F8 / Shift+F8 move through changed, missing or error files in the current sorted/filtered table. Hover over the preview for normalized baseline/current RGB and absolute channel deltas. Pixel coordinates use the image cell under the pointer. HTML export offers all, visible or selected results.

Folder mode also accepts `--json-report NEW.json` and `--junit-report NEW.xml`, optionally alongside HTML. JUnit maps changed/missing files to failures, decoder errors to errors, and accepted/identical images to passes. JSON schema 1 includes profile, relative paths, SHA256 identities and statistics, without absolute input paths. Reports refuse existing outputs and stale inputs/settings. Reports describe the bounded 8-bit RGB comparison, not color-managed/HDR equivalence.
