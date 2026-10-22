# Licensing and corresponding source

DiffImg's source headers permit GPL version 2 or (at your option) any later version. This Windows binary distribution selects the GPLv3 option; original GPLv2 text and all upstream notices are retained. Qt is dynamically linked and retains its own LGPLv3/GPL licensing choices and notices. No proprietary relicensing is claimed.

| Component | Version / origin | Notices and source |
|---|---|---|
| DiffImg | xbee → lingo → sandsmark → arbruijn; revival changes by Sean Shaffer, 2026 | GPL-2.0-or-later source headers; original LICENSE.txt, COPYING.txt, AUTHORS.txt and CREDITS.txt preserved |
| Qt runtime and plugins | Official Qt 5.15.2 MSVC2019 x64 open-source archives, only Qtbase runtime modules/codecs packaged | Qt LGPLv3/GPL license texts, third-party notices and complete Qtbase 5.15.2 source archive supplied with the release |
| Qwt | 6.3.0, commit 4ddf2f3878d0835f678da0f2aba1180f2ba208f6 | Qwt License 1.0, based on LGPL2.1 with exceptions; COPYING supplied; full source and build-option changes supplied |
| QtPropertyBrowser | Bundled upstream Qt Solutions source | BSD-style Nokia notices retained in each source; complete representative notice copied into portable licenses folder |
| PerceptualDiff | Bundled Yangli Hector Yee implementation | GPLv2-or-later notices retained; source included in application source package |
| qimageblitz-derived helpers | Bundled helpers in BaseMetric.cpp | Clarence Dang/Mike Gashler BSD-style notice retained; notice/source copied into portable licenses folder |
| Microsoft Visual C++ runtime | VS2022 x64 VC143 redistributable files | Proprietary Microsoft runtime, redistributed app-locally under Visual Studio redistribution terms; not part of the GPL-covered application source. Windows Universal CRT is an OS prerequisite. See https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files |

DiffImg is based in part on the work of the Qwt project (https://qwt.sourceforge.io/).

Release source bundles include the exact application source, Qwt source and matching Qtbase source. The Qt source contains additional copyright/license notices for third-party code such as PNG, JPEG, zlib, FreeType, HarfBuzz and double-conversion. Source references and hashes are recorded in dependencies.lock.json. Microsoft runtime source is not supplied and is not represented as open source.

The upstream artwork and bundled source remain attributed through the original credits and corresponding source. This is a scoped provenance review for this package, not a claim that unrelated upstream components or dependencies were audited for every distribution model.
