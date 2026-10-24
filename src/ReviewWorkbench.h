#pragma once
// GPL-2.0-or-later. DiffImg Windows revival, 2026.
#include "ReviewProject.h"
#include <QDialog>
#include <QWidget>
#include <atomic>
#include <functional>
class QLineEdit;
class QComboBox;
class QTableWidget;
class QLabel;
class QPushButton;
class QThread;
class QSlider;
class ReviewCanvas : public QWidget {
    Q_OBJECT
public:
    explicit ReviewCanvas(QWidget* parent = nullptr);
    void images(const QImage&, const QImage&, const QImage&, const QList<QRect>&);
    void setMode(int);
    void fit();
    void actual();
    void focusRegion(QRect);
    void setMasks(const QList<QRect>&);
    void setMaskEditing(bool);
    void setDivider(int);
    int divider() const
    {
        return m_divider;
    }
    QSize imageSize() const
    {
        return m_left.isNull() ? m_right.size() : m_left.size();
    }
signals:
    void maskAdded(QRect);
    void dividerChanged(int);
    void pixelInspected(QString);

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    QPoint pixel(QPointF) const;
    void zoom(double, QPointF);
    QImage m_left, m_right, m_diff;
    QList<QRect> m_masks;
    QRect m_region;
    QPoint m_start;
    QPointF m_offset, m_last;
    double m_scale = 1;
    int m_mode = 0, m_divider = 50;
    bool m_edit = false, m_drag = false, m_pan = false, m_swipe = false;
};
class ReviewWorkbench : public QDialog {
    Q_OBJECT
public:
    explicit ReviewWorkbench(QWidget* parent = nullptr);
    ~ReviewWorkbench();
    void setFolders(const QString&, const QString&);
    void startComparison();
    bool busy() const
    {
        return m_worker != nullptr;
    }
    const QList<Review::Entry>& results() const
    {
        return m_entries;
    }

protected:
    void closeEvent(QCloseEvent*) override;

private:
    void task(std::function<void()>, std::function<void()>, bool cancellable = false);
    void updateEnabled();
    void fillTable();
    void showEntry();
    void editProfile();
    void invalidate();
    QList<Review::Entry> selection() const;
    QLineEdit *m_baseline, *m_current, *m_search;
    QComboBox *m_filter, *m_mode;
    QTableWidget* m_table;
    QLabel *m_status, *m_profileLabel, *m_regionLabel;
    QPushButton *m_compare, *m_cancel, *m_export, *m_approve, *m_restore, *m_mask, *m_clear;
    QWidget* m_controls;
    ReviewCanvas* m_canvas;
    QSlider* m_slider;
    Review::Profile m_profile;
    QList<Review::Entry> m_entries;
    QThread* m_worker = nullptr;
    std::atomic_bool m_cancelled { false };
    bool m_valid = false, m_cancellable = false;
    int m_selected = -1, m_regionIndex = -1;
};
