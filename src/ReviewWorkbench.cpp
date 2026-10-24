// GPL-2.0-or-later. DiffImg Windows revival, 2026.
#include "ReviewWorkbench.h"
#include <QThread>
#include <QtWidgets>
#include <algorithm>
#include <cmath>
#include <memory>
static QString metricName(const QString& id)
{
    return QMap<QString, QString> { { "PerChannelMetric", "Per channel" },
        { "PerChannelMeanMetric", "Channel mean" }, { "PerLuminanceMetric", "Luminance" },
        { "PerceptualMetric", "Perceptual" } }
        .value(id, id);
}
ReviewCanvas::ReviewCanvas(QWidget* p)
    : QWidget(p)
{
    setMinimumSize(320, 240);
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName("Comparison preview");
    setAccessibleDescription(
        "Mouse wheel or plus and minus zoom. Arrow keys pan. F fits and 1 shows actual size. Drag the slider "
        "line to compare. In ignore mode drag to mark a rectangle.");
}
void ReviewCanvas::images(const QImage& a, const QImage& b, const QImage& d, const QList<QRect>& m)
{
    m_left = a;
    m_right = b;
    m_diff = d;
    m_masks = m;
    m_region = { };
    fit();
}
void ReviewCanvas::setMode(int mode)
{
    m_mode = mode;
    fit();
}
void ReviewCanvas::setMasks(const QList<QRect>& m)
{
    m_masks = m;
    update();
}
void ReviewCanvas::setMaskEditing(bool v)
{
    m_edit = v;
    setCursor(v ? Qt::CrossCursor : Qt::OpenHandCursor);
}
void ReviewCanvas::setDivider(int v)
{
    m_divider = qBound(0, v, 100);
    update();
}
void ReviewCanvas::fit()
{
    if (m_left.isNull() && m_right.isNull())
        return;
    QSize size = m_left.isNull() ? m_right.size() : m_left.size();
    const double viewportWidth = m_mode == 1 ? (width() - 12) / 2.0 : width();
    m_scale = qMin((viewportWidth - 20) / size.width(), (height() - 20) / double(size.height()));
    m_offset
        = QPointF((viewportWidth - size.width() * m_scale) / 2, (height() - size.height() * m_scale) / 2);
    update();
}
void ReviewCanvas::actual()
{
    m_scale = 1;
    m_offset = QPointF(10, 10);
    update();
}
void ReviewCanvas::focusRegion(QRect rect)
{
    if (rect.isEmpty())
        return;
    m_region = rect;
    const double viewportWidth = m_mode == 1 ? (width() - 12) / 2.0 : width();
    m_scale = qMin((viewportWidth - 40) / rect.width(), (height() - 40) / double(rect.height()));
    m_scale = qMin(m_scale, 16.0);
    m_offset = QPointF(viewportWidth / 2, height() / 2.0) - QPointF(rect.center()) * m_scale;
    update();
}
void ReviewCanvas::zoom(double factor, QPointF at)
{
    if (m_mode == 1 && at.x() >= (width() - 12) / 2.0 + 12)
        at.rx() -= (width() - 12) / 2.0 + 12;
    double next = qBound(0.000001, m_scale * factor, 64.0);
    m_offset = at - (at - m_offset) * (next / m_scale);
    m_scale = next;
    update();
}
QPoint ReviewCanvas::pixel(QPointF screen) const
{
    const double origin = (width() - 12) / 2.0 + 12;
    if (m_mode == 1 && screen.x() >= origin)
        screen.rx() -= origin;
    return ((screen - m_offset) / m_scale).toPoint();
}
void ReviewCanvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#202630"));
    if (m_left.isNull() && m_right.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Select a comparison to preview");
        return;
    }
    const auto base = m_left.isNull() ? m_right : m_left;
    const QRect area = base.rect();
    auto decorations = [&] {
        painter.setPen(QPen(QColor("#fbbf24"), 2 / m_scale, Qt::DashLine));
        painter.setBrush(QColor(251, 191, 36, 55));
        for (auto mask : m_masks)
            painter.drawRect(mask.intersected(area));
        if (m_drag)
            painter.drawRect(QRect(m_start, pixel(m_last)).normalized().intersected(area));
        if (!m_region.isEmpty()) {
            painter.setPen(QPen(QColor("#4ade80"), 3 / m_scale));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(m_region);
        }
    };
    if (m_mode == 1) {
        const double panelWidth = (width() - 12) / 2.0;
        for (int side = 0; side < 2; ++side) {
            const double origin = side * (panelWidth + 12);
            painter.save();
            painter.setClipRect(QRectF(origin, 0, panelWidth, height()));
            painter.translate(QPointF(origin, 0) + m_offset);
            painter.scale(m_scale, m_scale);
            painter.drawImage(0, 0, side ? (m_right.isNull() ? base : m_right) : base);
            decorations();
            painter.restore();
            painter.fillRect(QRectF(origin, 0, panelWidth, 22), QColor(0, 0, 0, 180));
            painter.setPen(Qt::white);
            painter.drawText(QRectF(origin + 5, 0, panelWidth - 10, 22), Qt::AlignLeft | Qt::AlignVCenter,
                side ? "Current" : "Baseline");
        }
        return;
    }
    painter.translate(m_offset);
    painter.scale(m_scale, m_scale);
    if (m_mode == 2)
        painter.drawImage(0, 0, m_diff.isNull() ? base : m_diff);
    else {
        painter.drawImage(0, 0, base);
        if (!m_right.isNull()) {
            if (m_mode == 0) {
                painter.save();
                painter.setClipRect(QRect(0, 0, area.width() * m_divider / 100, area.height()));
                painter.drawImage(0, 0, m_right);
                painter.restore();
                painter.setPen(QPen(QColor("#38bdf8"), 2 / m_scale));
                painter.drawLine(
                    area.width() * m_divider / 100, 0, area.width() * m_divider / 100, area.height());
            } else {
                painter.setOpacity(0.5);
                painter.drawImage(0, 0, m_right);
                painter.setOpacity(1);
            }
        }
    }
    decorations();
}
void ReviewCanvas::resizeEvent(QResizeEvent*)
{
    fit();
}
void ReviewCanvas::wheelEvent(QWheelEvent* e)
{
    zoom(e->angleDelta().y() > 0 ? 1.2 : 1 / 1.2, e->position());
    e->accept();
}
void ReviewCanvas::mousePressEvent(QMouseEvent* e)
{
    setFocus();
    m_last = e->position();
    m_start = pixel(e->position());
    QSize size = m_left.isNull() ? m_right.size() : m_left.size();
    if (e->button() == Qt::LeftButton && m_edit && QRect(QPoint(), size).contains(m_start))
        m_drag = true;
    else if (e->button() == Qt::LeftButton && m_mode == 0
        && std::abs(e->position().x() - (m_offset.x() + size.width() * m_scale * m_divider / 100)) < 12)
        m_swipe = true;
    else
        m_pan = true;
}
void ReviewCanvas::mouseMoveEvent(QMouseEvent* e)
{
    if (m_pan) {
        m_offset += e->position() - m_last;
    }
    if (m_swipe) {
        int w = (m_left.isNull() ? m_right : m_left).width();
        if (w > 0) {
            setDivider(qRound((e->position().x() - m_offset.x()) / m_scale / w * 100));
            emit dividerChanged(m_divider);
        }
    }
    m_last = e->position();
    update();
}
void ReviewCanvas::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_drag) {
        QSize size = m_left.isNull() ? m_right.size() : m_left.size();
        QRect mask = QRect(m_start, pixel(e->position())).normalized().intersected(QRect(QPoint(), size));
        if (!mask.isEmpty())
            emit maskAdded(mask);
    }
    m_drag = m_pan = m_swipe = false;
    update();
}
void ReviewCanvas::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        zoom(1.2, rect().center());
        break;
    case Qt::Key_Minus:
        zoom(1 / 1.2, rect().center());
        break;
    case Qt::Key_F:
        fit();
        break;
    case Qt::Key_1:
        actual();
        break;
    case Qt::Key_Left:
        m_offset.rx() += 30;
        break;
    case Qt::Key_Right:
        m_offset.rx() -= 30;
        break;
    case Qt::Key_Up:
        m_offset.ry() += 30;
        break;
    case Qt::Key_Down:
        m_offset.ry() -= 30;
        break;
    default:
        QWidget::keyPressEvent(e);
        return;
    }
    update();
}

ReviewWorkbench::ReviewWorkbench(QWidget* parent)
    : QDialog(parent)
{
    setObjectName("reviewWorkbench");
    setWindowTitle("DiffImg — Project review");
    auto available = QGuiApplication::primaryScreen()->availableGeometry();
    resize(qMin(1420, int(available.width() * 0.95)), qMin(940, int(available.height() * 0.9)));
    setWindowFlag(Qt::WindowMinMaxButtonsHint, true);
    auto layout = new QVBoxLayout(this);
    m_controls = new QWidget(this);
    auto controls = new QVBoxLayout(m_controls);
    controls->setContentsMargins(0, 0, 0, 0);
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_controls);
    layout->addWidget(scroll, 1);
    auto folders = new QGridLayout;
    controls->addLayout(folders);
    m_baseline = new QLineEdit;
    m_current = new QLineEdit;
    m_baseline->setObjectName("baselineFolder");
    m_current->setObjectName("currentFolder");
    for (int i = 0; i < 2; ++i) {
        auto edit = i ? m_current : m_baseline;
        auto label = new QLabel(i ? "&Current folder" : "&Baseline folder");
        label->setBuddy(edit);
        folders->addWidget(label, i, 0);
        folders->addWidget(edit, i, 1);
        auto browse = new QPushButton("Browse…");
        browse->setAccessibleName(i ? "Browse current folder" : "Browse baseline folder");
        folders->addWidget(browse, i, 2);
        connect(browse, &QPushButton::clicked, this, [this, edit] {
            auto dir = QFileDialog::getExistingDirectory(this, "Select image folder", edit->text());
            if (!dir.isEmpty())
                edit->setText(dir);
        });
        connect(edit, &QLineEdit::textChanged, this, [this] { invalidate(); });
    }
    auto buttons = new QHBoxLayout;
    controls->addLayout(buttons);
    auto button = [this, &buttons](QString text, QString name) {
        auto b = new QPushButton(text);
        b->setObjectName(name);
        buttons->addWidget(b);
        return b;
    };
    m_compare = button("&Compare folders", "compareFolders");
    m_compare->setShortcut(QKeySequence("Ctrl+Return"));
    auto edit = button("Edit &profile…", "editProfile");
    auto load = button("&Load profile…", "loadProfile");
    auto save = button("&Save profile…", "saveProfile");
    buttons = new QHBoxLayout;
    controls->addLayout(buttons);
    m_export = button("Export &HTML…", "exportReport");
    m_approve = button("&Approve selected…", "approveSelected");
    m_restore = button("&Restore backup…", "restoreBackup");
    m_profileLabel = new QLabel;
    m_profileLabel->setTextFormat(Qt::PlainText);
    controls->addWidget(m_profileLabel);
    m_profileLabel->setText("Profile: Default · Per channel · exact pixels");
    auto search = new QHBoxLayout;
    controls->addLayout(search);
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Filter filenames…");
    m_search->setAccessibleName("Filter filenames");
    m_filter = new QComboBox;
    m_filter->addItems({ "All results", "Changed", "Accepted", "Identical", "Missing baseline",
        "Missing current", "Error" });
    m_filter->setAccessibleName("Filter comparison status");
    search->addWidget(m_search, 1);
    search->addWidget(m_filter);
    auto split = new QSplitter(Qt::Vertical);
    controls->addWidget(split, 1);
    m_table = new QTableWidget(0, 5);
    m_table->setObjectName("comparisonResults");
    m_table->setHorizontalHeaderLabels(
        { "Relative filename", "Status", "Changed pixels", "Maximum error", "Mean error" });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->setAccessibleName("Folder comparison results");
    split->addWidget(m_table);
    auto preview = new QWidget;
    auto previewLayout = new QVBoxLayout(preview);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    auto tools = new QHBoxLayout;
    previewLayout->addLayout(tools);
    m_mode = new QComboBox;
    m_mode->addItems({ "Before/after slider", "Side by side (synchronized)", "Difference", "50% overlay" });
    m_mode->setAccessibleName("Comparison display mode");
    tools->addWidget(m_mode);
    auto fit = new QPushButton("Fit (F)"), actual = new QPushButton("100% (1)");
    tools->addWidget(fit);
    tools->addWidget(actual);
    m_mask = new QPushButton("Draw ignore region");
    m_mask->setCheckable(true);
    m_mask->setObjectName("drawIgnoreRegion");
    m_clear = new QPushButton("Clear image regions");
    tools->addWidget(m_mask);
    tools->addWidget(m_clear);
    auto navigation = new QHBoxLayout;
    previewLayout->addLayout(navigation);
    auto previous = new QPushButton("Previous change"), next = new QPushButton("Next change");
    previous->setShortcut(QKeySequence("Shift+F7"));
    next->setShortcut(QKeySequence("F7"));
    navigation->addWidget(previous);
    navigation->addWidget(next);
    m_regionLabel = new QLabel("No region selected");
    navigation->addWidget(m_regionLabel, 1);
    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, 100);
    m_slider->setValue(50);
    m_slider->setAccessibleName("Before and after divider percentage");
    previewLayout->addWidget(m_slider);
    m_canvas = new ReviewCanvas;
    m_canvas->setObjectName("reviewCanvas");
    previewLayout->addWidget(m_canvas, 1);
    split->addWidget(preview);
    split->setSizes({ 240, 560 });
    auto footer = new QHBoxLayout;
    layout->addLayout(footer);
    m_status = new QLabel("Choose baseline and current folders, then compare.");
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::PlainText);
    footer->addWidget(m_status, 1);
    m_cancel = new QPushButton("Cancel comparison");
    m_cancel->setEnabled(false);
    footer->addWidget(m_cancel);
    auto close = new QPushButton("Close");
    footer->addWidget(close);
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    connect(m_cancel, &QPushButton::clicked, this, [this] {
        m_cancelled = true;
        m_status->setText("Stopping after the current image…");
    });
    connect(m_compare, &QPushButton::clicked, this, &ReviewWorkbench::startComparison);
    connect(edit, &QPushButton::clicked, this, &ReviewWorkbench::editProfile);
    connect(load, &QPushButton::clicked, this, [this] {
        auto path
            = QFileDialog::getOpenFileName(this, "Load comparison profile", { }, "DiffImg profile (*.json)");
        if (path.isEmpty())
            return;
        try {
            m_profile = Review::Profile::load(path);
            invalidate();
            m_profileLabel->setText("Profile: " + m_profile.name + " · " + metricName(m_profile.metric)
                + " · recompare to apply");
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Profile", QString::fromUtf8(e.what()));
        }
    });
    connect(save, &QPushButton::clicked, this, [this] {
        auto path
            = QFileDialog::getSaveFileName(this, "Save comparison profile", { }, "DiffImg profile (*.json)");
        if (path.isEmpty())
            return;
        try {
            m_profile.save(path);
            m_status->setText("Profile saved, including image-specific and global ignore regions.");
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Profile", QString::fromUtf8(e.what()));
        }
    });
    connect(m_search, &QLineEdit::textChanged, this, &ReviewWorkbench::fillTable);
    connect(m_filter, &QComboBox::currentIndexChanged, this, &ReviewWorkbench::fillTable);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ReviewWorkbench::showEntry);
    connect(m_mode, &QComboBox::currentIndexChanged, m_canvas, &ReviewCanvas::setMode);
    connect(fit, &QPushButton::clicked, m_canvas, &ReviewCanvas::fit);
    connect(actual, &QPushButton::clicked, m_canvas, &ReviewCanvas::actual);
    connect(m_slider, &QSlider::valueChanged, m_canvas, &ReviewCanvas::setDivider);
    connect(m_canvas, &ReviewCanvas::dividerChanged, m_slider, &QSlider::setValue);
    connect(m_mask, &QPushButton::toggled, m_canvas, &ReviewCanvas::setMaskEditing);
    connect(m_canvas, &ReviewCanvas::maskAdded, this, [this](QRect rect) {
        if (m_selected < 0)
            return;
        auto key = m_entries[m_selected].relative;
        m_profile.masks[key].append(rect);
        m_canvas->setMasks(m_profile.masks.value("*") + m_profile.masks.value(key));
        invalidate();
        m_status->setText(
            "Ignore region added for " + key + ". Compare again to apply; save the profile to keep it.");
    });
    connect(m_clear, &QPushButton::clicked, this, [this] {
        if (m_selected >= 0) {
            m_profile.masks.remove(m_entries[m_selected].relative);
            m_canvas->setMasks(m_profile.masks.value("*"));
            invalidate();
            m_status->setText(
                "Image-specific regions cleared. Global regions, if any, remain. Compare again to apply.");
        }
    });
    auto navigate = [this](int direction) {
        if (m_selected < 0)
            return;
        auto list = m_entries[m_selected].regions;
        if (list.isEmpty()) {
            m_regionLabel->setText("No changed regions");
            return;
        }
        m_regionIndex = m_regionIndex < 0 ? (direction > 0 ? 0 : list.size() - 1)
                                          : (m_regionIndex + direction + list.size()) % list.size();
        m_canvas->focusRegion(list[m_regionIndex]);
        m_regionLabel->setText(QString("Region %1/%2 (32px tiles)").arg(m_regionIndex + 1).arg(list.size()));
    };
    connect(previous, &QPushButton::clicked, this, [navigate] { navigate(-1); });
    connect(next, &QPushButton::clicked, this, [navigate] { navigate(1); });
    connect(m_export, &QPushButton::clicked, this, [this] {
        auto path = QFileDialog::getSaveFileName(
            this, "Export a new standalone report", { }, "HTML report (*.html)");
        if (path.isEmpty())
            return;
        auto entries = m_entries;
        auto profile = m_profile;
        task([path, entries, profile] { Review::exportHtml(path, entries, profile); },
            [this, path] { m_status->setText("Standalone report written: " + path); });
    });
    connect(m_approve, &QPushButton::clicked, this, [this] {
        auto rows = selection();
        if (rows.isEmpty())
            return;
        QStringList names;
        for (const auto& e : rows)
            names << e.relative;
        auto answer = QMessageBox::question(this, "Approve baseline changes",
            QString(
                "Replace/add %1 baseline image(s)?\n\n%2\n\nOriginal files will be backed up under "
                ".diffimg-backups. Missing-current images are never deleted. Changed inputs will be refused.")
                .arg(rows.size())
                .arg(names.mid(0, 20).join('\n')),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
        auto a = m_baseline->text(), b = m_current->text();
        auto result = std::make_shared<QString>();
        task([a, b, rows, result] { *result = Review::approve(a, b, rows); },
            [this, result] {
                invalidate();
                m_status->setText(
                    "Baseline approved. Recoverable backup: " + *result + ". Compare again to refresh.");
            });
    });
    connect(m_restore, &QPushButton::clicked, this, [this] {
        auto path = QFileDialog::getOpenFileName(this, "Select backup manifest",
            QDir(m_baseline->text()).filePath(".diffimg-backups"), "Backup manifest (manifest.json)");
        if (path.isEmpty())
            return;
        if (QMessageBox::question(this, "Restore baseline backup",
                "Restore this transaction? Files changed since approval will be refused. Newly added "
                "baselines from this transaction will be removed.",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes)
            return;
        auto root = m_baseline->text();
        task([root, path] { Review::restore(root, path); },
            [this] {
                invalidate();
                m_status->setText("Backup restored. Compare again to refresh.");
            });
    });
    updateEnabled();
}
ReviewWorkbench::~ReviewWorkbench()
{
    m_cancelled = true;
    if (m_worker)
        m_worker->wait();
}
void ReviewWorkbench::closeEvent(QCloseEvent* e)
{
    if (busy()) {
        m_cancelled = true;
        m_status->setText(
            "Finishing the current operation before closing. Please close again when it completes.");
        e->ignore();
    } else
        e->accept();
}
void ReviewWorkbench::setFolders(const QString& a, const QString& b)
{
    m_baseline->setText(a);
    m_current->setText(b);
}
void ReviewWorkbench::invalidate()
{
    m_valid = false;
    updateEnabled();
}
void ReviewWorkbench::updateEnabled()
{
    m_controls->setEnabled(!busy());
    m_cancel->setEnabled(busy() && m_cancellable);
    m_export->setEnabled(m_valid && !m_entries.isEmpty());
    const auto selected = selection();
    const bool approvable = !selected.isEmpty()
        && std::all_of(selected.begin(), selected.end(),
            [](const Review::Entry& e) { return e.comparable() || e.status == "Missing baseline"; });
    m_approve->setEnabled(m_valid && approvable);
    m_restore->setEnabled(!m_baseline->text().isEmpty());
}
void ReviewWorkbench::task(std::function<void()> work, std::function<void()> success, bool cancellable)
{
    if (busy())
        return;
    m_cancellable = cancellable;
    m_cancelled = false;
    auto error = std::make_shared<QString>();
    m_worker = QThread::create([work, error] {
        try {
            work();
        } catch (const std::exception& e) {
            *error = QString::fromUtf8(e.what());
        }
    });
    auto thread = m_worker;
    connect(thread, &QThread::finished, this, [this, error, success] {
        m_worker = nullptr;
        updateEnabled();
        if (!error->isEmpty()) {
            m_status->setText(*error);
            QMessageBox::warning(this, "Project review", *error);
        } else
            success();
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    updateEnabled();
    thread->start();
}
void ReviewWorkbench::startComparison()
{
    if (busy())
        return;
    auto a = m_baseline->text(), b = m_current->text();
    auto profile = m_profile;
    auto results = std::make_shared<QList<Review::Entry>>();
    m_valid = false;
    m_status->setText("Finding images…");
    task(
        [this, a, b, profile, results] {
            *results = Review::discover(a, b, &m_cancelled);
            if (m_cancelled)
                return;
            if (results->isEmpty())
                throw std::runtime_error("No supported images found");
            for (int i = 0; i < results->size() && !m_cancelled; ++i) {
                (*results)[i] = Review::compare((*results)[i], profile);
                QMetaObject::invokeMethod(
                    this,
                    [this, i, count = results->size()] {
                        m_status->setText(QString("Compared %1/%2").arg(i + 1).arg(count));
                    },
                    Qt::QueuedConnection);
            }
        },
        [this, results] {
            if (m_cancelled) {
                m_status->setText("Comparison cancelled; partial results were discarded.");
                updateEnabled();
                return;
            }
            m_entries = *results;
            m_valid = true;
            m_selected = -1;
            fillTable();
            int changed = 0, errors = 0;
            for (const auto& e : m_entries) {
                changed += e.status == "Changed" || e.status.startsWith("Missing");
                errors += e.status == "Error";
            }
            m_status->setText(
                QString("%1 images · %2 changed/missing · %3 errors. Select rows to review or approve.")
                    .arg(m_entries.size())
                    .arg(changed)
                    .arg(errors));
            updateEnabled();
        },
        true);
}
QList<Review::Entry> ReviewWorkbench::selection() const
{
    QList<Review::Entry> rows;
    for (const auto& idx : m_table->selectionModel()->selectedRows()) {
        int id = m_table->item(idx.row(), 0)->data(Qt::UserRole).toInt();
        if (id >= 0 && id < m_entries.size())
            rows.append(m_entries[id]);
    }
    return rows;
}
void ReviewWorkbench::fillTable()
{
    QSignalBlocker guard(m_table);
    m_table->setSortingEnabled(false);
    m_table->setRowCount(0);
    for (int i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        if (!e.relative.contains(m_search->text(), Qt::CaseInsensitive)
            || (m_filter->currentIndex() > 0 && e.status != m_filter->currentText()))
            continue;
        int row = m_table->rowCount();
        m_table->insertRow(row);
        auto item = new QTableWidgetItem(e.relative);
        item->setData(Qt::UserRole, i);
        m_table->setItem(row, 0, item);
        m_table->setItem(row, 1, new QTableWidgetItem(e.status));
        for (int col = 2; col < 5; ++col) {
            auto value = new QTableWidgetItem;
            value->setData(Qt::DisplayRole,
                col == 2 ? QVariant(e.pixels) : (col == 3 ? QVariant(e.maxError) : QVariant(e.meanError)));
            m_table->setItem(row, col, value);
        }
        for (int col = 0; col < 5; ++col)
            m_table->item(row, col)->setToolTip(e.detail);
    }
    m_table->setSortingEnabled(true);
    m_selected = -1;
    m_canvas->images({ }, { }, { }, { });
    updateEnabled();
}
void ReviewWorkbench::showEntry()
{
    updateEnabled();
    auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty() || busy())
        return;
    int id = m_table->item(rows.first().row(), 0)->data(Qt::UserRole).toInt();
    if (id == m_selected)
        return;
    m_selected = id;
    m_regionIndex = -1;
    m_regionLabel->setText("F7 / Shift+F7 navigate changed regions");
    auto entry = m_entries[id];
    if (entry.status == "Error") {
        m_canvas->images({ }, { }, { }, { });
        m_status->setText(entry.detail);
        return;
    }
    auto profile = m_profile;
    auto images = std::make_shared<QList<QImage>>();
    task(
        [entry, profile, images] {
            QImage diff;
            if (entry.comparable()) {
                auto fresh = Review::compare(entry, profile, &diff);
                if (!fresh.comparable() || fresh.leftHash != entry.leftHash
                    || fresh.rightHash != entry.rightHash)
                    throw std::runtime_error("Inputs changed since scan; compare again");
            }
            images->append(entry.left.isEmpty() ? QImage() : Review::readImage(entry.left));
            images->append(entry.right.isEmpty() ? QImage() : Review::readImage(entry.right));
            images->append(diff);
        },
        [this, images, entry] {
            m_canvas->images(images->at(0), images->at(1), images->at(2),
                m_profile.masks.value("*") + m_profile.masks.value(entry.relative));
        });
}
void ReviewWorkbench::editProfile()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Comparison profile");
    auto layout = new QFormLayout(&dialog);
    QLineEdit name(m_profile.name);
    QComboBox metric;
    for (const QString& id :
        { "PerChannelMetric", "PerChannelMeanMetric", "PerLuminanceMetric", "PerceptualMetric" })
        metric.addItem(metricName(id), id);
    metric.setCurrentIndex(metric.findData(m_profile.metric));
    QSpinBox pixels, maximum;
    pixels.setRange(0, 16777216);
    pixels.setValue(m_profile.allowedPixels);
    maximum.setRange(0, 255);
    maximum.setValue(m_profile.maxError);
    QDoubleSpinBox mean;
    mean.setDecimals(5);
    mean.setRange(0, 255);
    mean.setValue(m_profile.meanError);
    layout->addRow("Profile &name", &name);
    layout->addRow("&Metric", &metric);
    layout->addRow("Allowed changed &pixels", &pixels);
    layout->addRow("Maximum channel &error", &maximum);
    layout->addRow("Maximum mean error", &mean);
    QDoubleSpinBox fov, gamma, luminance, color;
    QCheckBox only;
    fov.setRange(0.1, 89.9);
    fov.setValue(m_profile.parameters.value("FOV", 45));
    gamma.setRange(0.001, 10);
    gamma.setDecimals(3);
    gamma.setValue(m_profile.parameters.value("Gamma", 2.2));
    luminance.setRange(0.001, 10000);
    luminance.setDecimals(3);
    luminance.setValue(m_profile.parameters.value("Luminance", 100));
    color.setRange(0, 1);
    color.setDecimals(3);
    color.setValue(m_profile.parameters.value("ColorFactor", 1));
    only.setChecked(m_profile.parameters.value("LuminanceOnly", 0));
    layout->addRow("Perceptual field of view", &fov);
    layout->addRow("Perceptual gamma", &gamma);
    layout->addRow("Perceptual luminance", &luminance);
    layout->addRow("Perceptual color factor", &color);
    layout->addRow("Perceptual luminance only", &only);
    QLabel hint("Ignore regions are saved in image pixel coordinates. Drag on the preview to add them, or "
                "enter a global rectangle below. All thresholds must pass.");
    hint.setWordWrap(true);
    layout->addRow(&hint);
    QLineEdit global;
    global.setPlaceholderText("Global region: x,y,width,height (optional)");
    global.setAccessibleName("Global ignore rectangle");
    layout->addRow("Add global region", &global);
    QCheckBox clearGlobal("Clear existing global regions");
    layout->addRow(&clearGlobal);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        try {
            auto p = m_profile;
            p.name = name.text();
            p.metric = metric.currentData().toString();
            p.allowedPixels = pixels.value();
            p.maxError = maximum.value();
            p.meanError = mean.value();
            p.parameters.clear();
            if (p.metric == "PerceptualMetric")
                p.parameters = { { "FOV", fov.value() }, { "Gamma", gamma.value() },
                    { "Luminance", luminance.value() }, { "ColorFactor", color.value() },
                    { "LuminanceOnly", only.isChecked() ? 1.0 : 0.0 } };
            if (clearGlobal.isChecked())
                p.masks.remove("*");
            if (!global.text().trimmed().isEmpty()) {
                auto values = global.text().split(',');
                if (values.size() != 4)
                    throw std::runtime_error("Global rectangle requires x,y,width,height");
                QList<int> numbers;
                for (auto v : values) {
                    bool ok;
                    int n = v.trimmed().toInt(&ok);
                    if (!ok)
                        throw std::runtime_error("Rectangle values must be integers");
                    numbers << n;
                }
                p.masks["*"].append(QRect(numbers[0], numbers[1], numbers[2], numbers[3]));
            }
            p.validate();
            m_profile = p;
            dialog.accept();
        } catch (const std::exception& e) {
            QMessageBox::warning(&dialog, "Profile", QString::fromUtf8(e.what()));
        }
    });
    if (dialog.exec() == QDialog::Accepted) {
        invalidate();
        m_profileLabel->setText(
            "Profile: " + m_profile.name + " · " + metricName(m_profile.metric) + " · recompare to apply");
        m_status->setText("Profile changed; compare again before exporting or approving.");
    }
}
