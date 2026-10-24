// GPL-2.0-or-later. DiffImg Windows revival, 2026.
#include "ReviewProject.h"
#include "PerChannelMeanMetric.h"
#include "PerChannelMetric.h"
#include "PerLuminanceMetric.h"
#include "PerceptualMetric.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QRegion>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
namespace Review {
static void fail(const QString& s)
{
    throw std::runtime_error(s.toUtf8().constData());
}
static void require(bool b, const QString& s)
{
    if (!b)
        fail(s);
}
static int integer(QJsonValue v, int minimum = 0, int maximum = 16777216)
{
    require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() == std::floor(v.toDouble())
            && v.toDouble() >= minimum && v.toDouble() <= maximum,
        "Invalid profile integer");
    return v.toInt();
}
static void relativeSafe(const QString& s)
{
    require(!s.isEmpty() && !QDir::isAbsolutePath(s) && !s.contains('\\') && !s.contains(':')
            && !s.split('/').contains("..") && !s.split('/').contains(".")
            && !s.split('/').contains(".diffimg-backups", Qt::CaseInsensitive),
        "Unsafe relative image path");
}
static QString rootPath(const QString& path)
{
    QFileInfo info(path);
    require(info.isDir() && !info.isSymLink() && !info.isJunction(), "Choose an existing, non-linked folder");
    return info.canonicalFilePath();
}
static bool samePath(const QString& a, const QString& b)
{
#ifdef Q_OS_WIN
    return a.compare(b, Qt::CaseInsensitive) == 0;
#else
    return a == b;
#endif
}
static bool inside(const QString& path, const QString& root)
{
#ifdef Q_OS_WIN
    return path.startsWith(root + "/", Qt::CaseInsensitive);
#else
    return path.startsWith(root + "/");
#endif
}
static QString safePath(const QString& root, const QString& relative)
{
    relativeSafe(relative);
    QString path = QDir(root).filePath(relative);
    QFileInfo info(path);
    QString ancestor = path;
    while (inside(ancestor, root)) {
        require(!QFileInfo(ancestor).isSymLink() && !QFileInfo(ancestor).isJunction(),
            "Linked paths cannot be modified");
        ancestor = QFileInfo(ancestor).path();
    }
    if (info.exists())
        require(inside(info.canonicalFilePath(), root), "Path escapes selected folder");
    return path;
}
void Profile::validate() const
{
    require(
        !name.trimmed().isEmpty() && name.size() <= 200, "Profile name is required (maximum 200 characters)");
    require(
        QStringList { "PerChannelMetric", "PerChannelMeanMetric", "PerLuminanceMetric", "PerceptualMetric" }
            .contains(metric),
        "Unknown comparison metric");
    require(allowedPixels >= 0 && allowedPixels <= 16777216 && maxError >= 0 && maxError <= 255
            && std::isfinite(meanError) && meanError >= 0 && meanError <= 255,
        "Invalid comparison thresholds");
    const QMap<QString, QPair<double, double>> ranges
        = { { "FOV", { 0.1, 89.9 } }, { "Gamma", { 0.001, 10 } }, { "Luminance", { 0.001, 10000 } },
              { "LuminanceOnly", { 0, 1 } }, { "ColorFactor", { 0, 1 } }, { "Downsample", { 0, 0 } } };
    for (auto i = parameters.begin(); i != parameters.end(); ++i) {
        require(metric == "PerceptualMetric" && ranges.contains(i.key()) && std::isfinite(i.value())
                && i.value() >= ranges[i.key()].first && i.value() <= ranges[i.key()].second,
            "Invalid metric parameter: " + i.key());
        if (i.key() == "LuminanceOnly")
            require(i.value() == 0 || i.value() == 1, "LuminanceOnly must be 0 or 1");
    }
    int count = 0;
    for (auto i = masks.begin(); i != masks.end(); ++i) {
        if (i.key() != "*")
            relativeSafe(i.key());
        for (auto rect : i.value()) {
            ++count;
            require(rect.x() >= 0 && rect.y() >= 0 && rect.width() > 0 && rect.height() > 0
                    && qint64(rect.x()) + rect.width() <= 16777216
                    && qint64(rect.y()) + rect.height() <= 16777216,
                "Invalid ignore rectangle");
        }
    }
    require(count <= 1000, "Maximum 1000 ignore rectangles per profile");
}
QJsonObject Profile::json() const
{
    validate();
    QJsonObject regions, params;
    for (auto i = masks.begin(); i != masks.end(); ++i) {
        QJsonArray array;
        for (auto r : i.value())
            array.append(QJsonArray { r.x(), r.y(), r.width(), r.height() });
        regions[i.key()] = array;
    }
    for (auto i = parameters.begin(); i != parameters.end(); ++i)
        params[i.key()] = i.value();
    return { { "schema", 1 }, { "name", name }, { "metric", metric }, { "allowedPixels", allowedPixels },
        { "maxError", maxError }, { "meanError", meanError }, { "parameters", params },
        { "masks", regions } };
}
Profile Profile::fromJson(const QJsonObject& o)
{
    require(integer(o["schema"], 1, 1) == 1, "Unsupported profile schema");
    const QSet<QString> keys
        = { "schema", "name", "metric", "allowedPixels", "maxError", "meanError", "parameters", "masks" };
    for (auto i = o.begin(); i != o.end(); ++i)
        require(keys.contains(i.key()), "Unknown profile field: " + i.key());
    require(o["name"].isString() && o["metric"].isString() && o["meanError"].isDouble()
            && o["parameters"].isObject() && o["masks"].isObject(),
        "Invalid profile fields");
    Profile p;
    p.name = o["name"].toString();
    p.metric = o["metric"].toString();
    p.allowedPixels = integer(o["allowedPixels"]);
    p.maxError = integer(o["maxError"], 0, 255);
    p.meanError = o["meanError"].toDouble();
    auto params = o["parameters"].toObject();
    for (auto i = params.begin(); i != params.end(); ++i) {
        require(i.value().isDouble(), "Metric parameters must be numeric");
        p.parameters[i.key()] = i.value().toDouble();
    }
    auto masks = o["masks"].toObject();
    for (auto i = masks.begin(); i != masks.end(); ++i) {
        require(i.value().isArray(), "Invalid masks");
        for (auto v : i.value().toArray()) {
            require(v.isArray() && v.toArray().size() == 4, "Rectangle requires x,y,width,height");
            auto a = v.toArray();
            p.masks[i.key()].append(QRect(integer(a[0]), integer(a[1]), integer(a[2], 1), integer(a[3], 1)));
        }
    }
    p.validate();
    return p;
}
static QJsonObject readJson(const QString& file, qint64 limit = 1048576)
{
    QFile f(file);
    require(
        f.open(QIODevice::ReadOnly) && f.size() <= limit, "Cannot read JSON file or file exceeds size limit");
    QJsonParseError e;
    auto d = QJsonDocument::fromJson(f.readAll(), &e);
    require(e.error == QJsonParseError::NoError && d.isObject(), "Invalid JSON document");
    return d.object();
}
Profile Profile::load(const QString& f)
{
    return fromJson(readJson(f));
}
static void writeAtomic(const QString& path, const QByteArray& data)
{
    QSaveFile f(path);
    require(f.open(QIODevice::WriteOnly) && f.write(data) == data.size() && f.commit(),
        "Could not write file: " + QFileInfo(path).fileName());
}
void Profile::save(const QString& f) const
{
    auto data = QJsonDocument(json()).toJson();
    require(data.size() <= 1048576, "Profile exceeds 1 MiB");
    writeAtomic(f, data);
}
QByteArray hashFile(const QString& path)
{
    QFile f(path);
    require(
        f.open(QIODevice::ReadOnly) && f.size() <= 268435456, "Cannot read image or image exceeds 256 MiB");
    QCryptographicHash h(QCryptographicHash::Sha256);
    require(h.addData(&f), "Could not hash image");
    return h.result().toHex();
}
QImage readImage(const QString& path)
{
    QImageReader r(path);
    auto s = r.size();
    require(s.isValid() && qint64(s.width()) * s.height() <= 16777216,
        "Unreadable image or image exceeds 16,777,216 pixels");
    auto img = r.read();
    require(!img.isNull(), "Image decoding failed");
    return BaseMetric::comparisonImage(img);
}
QList<Entry> discover(const QString& left, const QString& right, const std::atomic_bool* cancel)
{
    const QString a = rootPath(left), b = rootPath(right);
    require(
        a != b && !inside(a, b) && !inside(b, a), "Choose separate folders that do not contain one another");
    QMap<QString, Entry> pairs;
    for (int side = 0; side < 2; ++side) {
        QString root = side ? b : a;
        QDirIterator it(root, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        int count = 0;
        while (it.hasNext()) {
            if (cancel && *cancel)
                return { };
            QString file = it.next(), rel = QDir(root).relativeFilePath(file);
            if (rel.split('/').contains(".diffimg-backups", Qt::CaseInsensitive))
                continue;
            const QString ext = QFileInfo(file).suffix().toLower();
            if (!QStringList { "png", "bmp", "jpg", "jpeg", "gif", "ico" }.contains(ext))
                continue;
            safePath(root, rel);
            require(++count <= 20000, "Maximum 20,000 images per folder");
            relativeSafe(rel);
            QString key = rel;
#ifdef Q_OS_WIN
            key = key.toCaseFolded();
#endif
            auto& entry = pairs[key];
            if (entry.relative.isEmpty())
                entry.relative = rel;
            require((side ? entry.right : entry.left).isEmpty(), "Ambiguous image filename");
            (side ? entry.right : entry.left) = file;
        }
    }
    return pairs.values();
}
static QList<QRect> regions(const QImage& diff)
{
    const int cols = (diff.width() + 31) / 32, rows = (diff.height() + 31) / 32;
    QSet<int> cells;
    for (int y = 0; y < diff.height(); ++y) {
        const auto* pixels = reinterpret_cast<const QRgb*>(diff.constScanLine(y));
        for (int x = 0; x < diff.width(); ++x)
            if ((pixels[x] & 0xffffff) != 0)
                cells.insert((y / 32) * cols + x / 32);
    }
    QList<QRect> result;
    while (!cells.isEmpty()) {
        int seed = *cells.begin();
        cells.remove(seed);
        QList<int> todo { seed };
        QRect bounds((seed % cols) * 32, (seed / cols) * 32, 32, 32);
        while (!todo.isEmpty()) {
            int cell = todo.takeLast(), x = cell % cols, y = cell / cols;
            bounds = bounds.united(QRect(x * 32, y * 32, 32, 32));
            for (auto p : { QPoint(x - 1, y), QPoint(x + 1, y), QPoint(x, y - 1), QPoint(x, y + 1) })
                if (p.x() >= 0 && p.x() < cols && p.y() >= 0 && p.y() < rows) {
                    int n = p.y() * cols + p.x();
                    if (cells.remove(n))
                        todo.append(n);
                }
        }
        result.append(bounds.intersected(diff.rect()));
    }
    std::sort(result.begin(), result.end(),
        [](QRect a, QRect b) { return a.top() == b.top() ? a.left() < b.left() : a.top() < b.top(); });
    return result;
}
Entry compare(Entry e, const Profile& profile, QImage* difference)
{
    profile.validate();
    e.status.clear();
    e.detail.clear();
    e.regions.clear();
    e.pixels = 0;
    e.maxError = 0;
    e.meanError = 0;
    e.size = { };
    if (difference)
        *difference = { };
    try {
        e.leftHash = e.left.isEmpty() ? QByteArray() : hashFile(e.left);
        e.rightHash = e.right.isEmpty() ? QByteArray() : hashFile(e.right);
        if (e.left.isEmpty()) {
            readImage(e.right);
            e.status = "Missing baseline";
            return e;
        }
        if (e.right.isEmpty()) {
            readImage(e.left);
            e.status = "Missing current";
            return e;
        }
        std::unique_ptr<BaseMetric> metric;
        if (profile.metric == "PerChannelMetric")
            metric = std::make_unique<PerChannelMetric>();
        else if (profile.metric == "PerChannelMeanMetric")
            metric = std::make_unique<PerChannelMeanMetric>();
        else if (profile.metric == "PerLuminanceMetric")
            metric = std::make_unique<PerLuminanceMetric>();
        else
            metric = std::make_unique<PerceptualMetric>();
        metric->setPersistSettings(false);
        metric->getInputParams();
        for (auto i = profile.parameters.begin(); i != profile.parameters.end(); ++i)
            metric->getInputParam(i.key())->threshold = i.value();
        metric->setIgnoreRegions(profile.masks.value("*") + profile.masks.value(e.relative));
        metric->checkDifferences(e.left, e.right);
        require(metric->isValid(), "Comparison failed: check dimensions, image decoding and metric settings");
        require(hashFile(e.left) == e.leftHash && hashFile(e.right) == e.rightHash,
            "Inputs changed during comparison; rescan required");
        e.size = metric->getImageDifference().size();
        e.pixels = metric->getPixelError();
        e.maxError = metric->getMaxError();
        quint64 channelSum = 0;
        const QImage differenceImage = metric->getImageDifference().convertToFormat(QImage::Format_RGB32);
        for (int y = 0; y < differenceImage.height(); ++y) {
            const auto* row = reinterpret_cast<const QRgb*>(differenceImage.constScanLine(y));
            for (int x = 0; x < differenceImage.width(); ++x)
                channelSum += qRed(row[x]) + qGreen(row[x]) + qBlue(row[x]);
        }
        QRegion ignored;
        for (auto mask : profile.masks.value("*") + profile.masks.value(e.relative))
            ignored += mask.intersected(QRect(QPoint(), e.size));
        qint64 ignoredPixels = 0;
        for (auto rect : ignored)
            ignoredPixels += qint64(rect.width()) * rect.height();
        qint64 total = qint64(e.size.width()) * e.size.height();
        e.meanError = total == ignoredPixels ? 0 : double(channelSum) / (3.0 * double(total - ignoredPixels));
        e.status = e.pixels == 0 ? "Identical"
                                 : (e.pixels <= profile.allowedPixels && e.maxError <= profile.maxError
                                               && e.meanError <= profile.meanError
                                           ? "Accepted"
                                           : "Changed");
        e.regions = regions(metric->getImageDifference());
        if (difference)
            *difference = metric->getImageDifference();
    } catch (const std::exception& error) {
        e.status = "Error";
        e.detail = QString::fromUtf8(error.what());
    }
    return e;
}
static QString imageData(QImage image)
{
    image = image.scaled(1600, 1600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    require(image.save(&buffer, "PNG"), "Could not encode report preview");
    return QString::fromLatin1(data.toBase64());
}
void exportHtml(const QString& file, const QList<Entry>& entries, const Profile& profile)
{
    profile.validate();
    require(!QFileInfo::exists(file), "Report output already exists; choose a new filename");
    QString html
        = "<!doctype html><html lang='en'><meta charset='utf-8'><meta name='viewport' "
          "content='width=device-width'><meta http-equiv='Content-Security-Policy' content=\"default-src "
          "'none'; img-src data:; style-src 'unsafe-inline'\"><title>DiffImg "
          "report</title><style>body{font:16px "
          "system-ui;margin:2em;background:#15191f;color:#edf0f5}section{border-top:1px solid "
          "#657080;padding:1em "
          "0}figure{display:inline-block;width:30%;vertical-align:top;margin:1%}img{width:100%;background:"
          "white}pre{white-space:pre-wrap}h2{overflow-wrap:anywhere}</style><h1>DiffImg comparison "
          "report</h1><p>8-bit RGB over white; equal dimensions. Preview images are scaled to at most 1600 "
          "pixels. Ignore regions are applied to comparison results.</p><h2>Profile</h2><pre>"
        + QString::fromUtf8(QJsonDocument(profile.json()).toJson()).toHtmlEscaped() + "</pre>";
    QMap<QString, int> counts;
    for (const auto& e : entries)
        ++counts[e.status];
    for (auto i = counts.begin(); i != counts.end(); ++i)
        html += "<p>" + i.key().toHtmlEscaped() + ": " + QString::number(i.value()) + "</p>";
    int number = 0;
    for (const auto& entry : entries) {
        require(++number <= 20000, "Too many report entries");
        html += "<section><h2>" + entry.relative.toHtmlEscaped() + " — " + entry.status.toHtmlEscaped()
            + "</h2>";
        html += "<p>Changed pixels: " + QString::number(entry.pixels) + "; maximum channel error: "
            + QString::number(entry.maxError) + "; mean error: " + QString::number(entry.meanError, 'g', 8)
            + "</p><p>" + entry.detail.toHtmlEscaped() + "</p>";
        if (entry.comparable()) {
            require(hashFile(entry.left) == entry.leftHash && hashFile(entry.right) == entry.rightHash,
                "Inputs changed since scan; rescan before reporting");
            QImage diff;
            auto fresh = compare(entry, profile, &diff);
            require(fresh.comparable(), "Could not refresh report images");
            require(fresh.status == entry.status && fresh.pixels == entry.pixels && fresh.maxError == entry.maxError && fresh.meanError == entry.meanError,
                "Comparison profile changed; rescan before reporting");
            for (auto pair : { qMakePair(QString("Baseline"), readImage(entry.left)),
                     qMakePair(QString("Current"), readImage(entry.right)),
                     qMakePair(QString("Difference"), diff) })
                html += "<figure><figcaption>" + pair.first + "</figcaption><img alt='" + pair.first
                    + "' src='data:image/png;base64," + imageData(pair.second) + "'></figure>";
        }
        if (entry.comparable())
            require(hashFile(entry.left) == entry.leftHash && hashFile(entry.right) == entry.rightHash,
                "Inputs changed while exporting the report");
        html += "</section>";
        require(html.size() < 100000000, "Report exceeds 100 MB; select fewer entries");
    }
    html += "</html>";
    QFile output(file);
    require(output.open(QIODevice::WriteOnly | QIODevice::NewOnly), "Could not create a new report file");
    auto bytes = html.toUtf8();
    if (output.write(bytes) != bytes.size()) {
        output.close();
        QFile::remove(file);
        fail("Could not finish report");
    }
}
static QByteArray bytes(const QString& path)
{
    QFile f(path);
    require(f.open(QIODevice::ReadOnly) && f.size() <= 268435456, "Could not read image bytes");
    return f.readAll();
}
QString approve(const QString& baseline, const QString& current, const QList<Entry>& selected)
{
    QString a = rootPath(baseline), b = rootPath(current);
    require(!samePath(a, b) && !inside(a, b) && !inside(b, a) && !selected.isEmpty(),
        "Select current images to approve in separate folders");
    QLockFile lock(a + "/.diffimg-approval.lock");
    require(lock.tryLock(0), "Another baseline operation is running");
    QString backupRoot = QDir(a).filePath(".diffimg-backups");
    require(
        !QFileInfo(backupRoot).isSymLink() && !QFileInfo(backupRoot).isJunction(), "Backup folder is linked");
    require(QDir().mkpath(backupRoot), "Cannot create backup folder");
    QString backup = backupRoot + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    require(QDir().mkpath(backup), "Cannot create backup transaction");
    QJsonArray records;
    QSet<QString> seen;
    for (const auto& e : selected) {
        relativeSafe(e.relative);
        require(!seen.contains(e.relative.toCaseFolded()), "Duplicate approval selection");
        seen.insert(e.relative.toCaseFolded());
        require(e.comparable() || e.status == "Missing baseline",
            "Cannot approve missing-current or invalid images");
        const QString target = safePath(a, e.relative);
        QString sourceRel = QDir(b).relativeFilePath(e.right);
        require(safePath(b, sourceRel) == e.right, "Current image is outside its folder");
        require(hashFile(e.right) == e.rightHash, "Current image changed since scan; rescan required");
        readImage(e.right);
        bool exists = QFileInfo::exists(target);
        require(exists == !e.left.isEmpty(), "Baseline existence changed since scan");
        if (exists)
            require(hashFile(target) == e.leftHash, "Baseline changed since scan; rescan required");
        QString saved = QString::number(records.size()) + ".original";
        if (exists)
            writeAtomic(backup + "/" + saved, bytes(target));
        records.append(QJsonObject { { "path", e.relative }, { "original", saved }, { "existed", exists },
            { "before", QString::fromLatin1(e.leftHash) }, { "after", QString::fromLatin1(e.rightHash) },
            { "source", sourceRel } });
    }
    QString manifest = backup + "/manifest.json";
    writeAtomic(manifest,
        QJsonDocument(QJsonObject { { "schema", 1 }, { "state", "prepared" }, { "files", records } })
            .toJson());
    QList<QJsonObject> applied;
    try {
        for (auto value : records) {
            auto o = value.toObject();
            QString target = safePath(a, o["path"].toString());
            require(QFileInfo::exists(target) == o["existed"].toBool(), "Baseline changed while approving");
            if (o["existed"].toBool())
                require(hashFile(target) == o["before"].toString().toLatin1(),
                    "Baseline changed while approving");
            QString source = safePath(b, o["source"].toString());
            QByteArray data = bytes(source);
            require(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex()
                    == o["after"].toString().toLatin1(),
                "Current image changed while approving");
            require(QDir().mkpath(QFileInfo(target).path()), "Cannot create baseline directory");
            writeAtomic(target, data);
            applied.append(o);
        }
    } catch (...) {
        for (const auto& o : applied) {
            auto target = safePath(a, o["path"].toString());
            if (hashFile(target) == o["after"].toString().toLatin1()) {
                if (o["existed"].toBool())
                    writeAtomic(target, bytes(backup + "/" + o["original"].toString()));
                else
                    QFile::remove(target);
            }
        }
        throw;
    }
    writeAtomic(manifest,
        QJsonDocument(QJsonObject { { "schema", 1 }, { "state", "committed" }, { "files", records } })
            .toJson());
    return manifest;
}
void restore(const QString& baseline, const QString& manifest)
{
    QString root = rootPath(baseline);
    QLockFile lock(root + "/.diffimg-approval.lock");
    require(lock.tryLock(0), "Another baseline operation is running");
    auto info = QFileInfo(manifest);
    require(!info.isSymLink() && !info.isJunction()
            && inside(info.canonicalFilePath(), root + "/.diffimg-backups"),
        "Choose a backup manifest inside this baseline folder");
    auto o = readJson(manifest, 16777216);
    require(o["schema"].toInt() == 1
            && (o["state"].toString() == "committed" || o["state"].toString() == "prepared")
            && o["files"].isArray(),
        "Backup is not a committed transaction");
    auto files = o["files"].toArray();
    QSet<QString> paths;
    for (auto value : files) {
        auto record = value.toObject();
        QString rel = record["path"].toString();
        relativeSafe(rel);
        require(!paths.contains(rel.toCaseFolded()), "Duplicate backup path");
        paths.insert(rel.toCaseFolded());
        QString target = safePath(root, rel);
        QByteArray actual = QFileInfo::exists(target) ? hashFile(target) : QByteArray();
        require(actual == record["after"].toString().toLatin1()
                || (record["existed"].toBool() && actual == record["before"].toString().toLatin1())
                || (!record["existed"].toBool() && actual.isEmpty()),
            "Baseline changed after approval; restoration refused");
        if (record["existed"].toBool()) {
            QString original = record["original"].toString();
            require(original == QFileInfo(original).fileName() && !original.contains(':'),
                "Unsafe backup filename");
            QString saved = safePath(info.path(), original);
            require(hashFile(saved) == record["before"].toString().toLatin1(), "Backup data is corrupt");
        }
    }
    for (auto value : files) {
        auto record = value.toObject();
        QString target = safePath(root, record["path"].toString());
        QByteArray actual = QFileInfo::exists(target) ? hashFile(target) : QByteArray();
        if ((record["existed"].toBool() && actual == record["before"].toString().toLatin1())
            || (!record["existed"].toBool() && actual.isEmpty()))
            continue;
        require(actual == record["after"].toString().toLatin1(), "Baseline changed during restoration");
        if (record["existed"].toBool())
            writeAtomic(target, bytes(safePath(info.path(), record["original"].toString())));
        else
            require(QFile::remove(target), "Could not remove newly approved baseline");
    }
    o["state"] = "restored";
    writeAtomic(manifest, QJsonDocument(o).toJson());
}
int batch(const QStringList& args)
{
    try {
        QString baseline, current, profileFile, report;
        for (int i = 1; i < args.size(); ++i) {
            auto arg = args[i];
            if (arg == "--folder-batch")
                continue;
            if (arg == "--baseline" || arg == "--current" || arg == "--profile" || arg == "--report") {
                require(i + 1 < args.size() && !args[i + 1].startsWith("--"), "Missing folder option value");
                QString value = args[++i];
                if (arg == "--baseline")
                    baseline = value;
                else if (arg == "--current")
                    current = value;
                else if (arg == "--profile")
                    profileFile = value;
                else
                    report = value;
            } else
                fail("Unknown folder option: " + arg);
        }
        require(!baseline.isEmpty() && !current.isEmpty(),
            "Use --folder-batch --baseline FOLDER --current FOLDER [--profile FILE] [--report NEW.html]");
        Profile p = profileFile.isEmpty() ? Profile() : Profile::load(profileFile);
        auto entries = discover(baseline, current);
        require(!entries.isEmpty(), "No supported images found");
        bool changed = false, error = false;
        QTextStream out(stdout);
        for (auto& e : entries) {
            e = compare(e, p);
            changed |= e.status == "Changed" || e.status.startsWith("Missing");
            error |= e.status == "Error";
            out << e.status << "\t" << e.relative << "\t" << e.pixels << Qt::endl;
        }
        if (!report.isEmpty()) {
            QString canonical = QFileInfo(report).absoluteFilePath();
            require(!inside(canonical, rootPath(baseline)) && !inside(canonical, rootPath(current)),
                "Save reports outside input folders");
            exportHtml(report, entries, p);
        }
        return error ? 2 : (changed ? 1 : 0);
    } catch (const std::exception& e) {
        QTextStream(stderr) << QString::fromUtf8(e.what()) << Qt::endl;
        return 2;
    }
}
}
