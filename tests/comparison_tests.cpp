#include <QtTest/QtTest>
#include <cmath>
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include "BaseMetric.h"
#include "PerChannelMetric.h"
#include "PerChannelMeanMetric.h"
#include "PerLuminanceMetric.h"
#include "PerceptualMetric.h"
#include "DiffImgWindow.h"
#include "MetricsRegistering.h"
#include <QGraphicsView>
#include <QDockWidget>
#include "AppSettings.h"

class ComparisonTests : public QObject {
    Q_OBJECT
    QTemporaryDir files;
    QString write(const QString &name, const QImage &image) {
        const QString path = files.filePath(name);
        if (!image.save(path)) qFatal("Could not create fixture");
        return path;
    }
    QImage solid(QRgb color, QSize size=QSize(3,2)) {
        QImage image(size,QImage::Format_ARGB32);
        image.fill(color);
        return image;
    }
private slots:
    void identical() {
        PerChannelMetric metric;
        const QString path = write("identical.png",solid(qRgb(21,43,65)));
        metric.checkDifferences(path,path);
        QVERIFY(metric.isValid());
        QCOMPARE(metric.getPixelError(),0);
        QCOMPARE(metric.getOutputParam("RMS")->value.toDouble(),0.0);
        QCOMPARE(qAlpha(metric.getImageMask().pixel(0,0)),0);
    }
    void singlePixelNumericalOracle() {
        QImage left=solid(qRgb(0,0,0));
        QImage right=left;
        right.setPixel(1,1,qRgb(30,60,90));
        PerChannelMetric metric;
        metric.checkDifferences(write("oracle-left.png",left),write("oracle-right.png",right));
        QCOMPARE(metric.getPixelError(),1);
        QCOMPARE(metric.getImageDifference().pixel(1,1),qRgb(30,60,90));
        QCOMPARE(metric.getOutputParam("MeanError")->value.toDouble(),10.0);
        QVERIFY(qAbs(metric.getOutputParam("RMS")->value.toDouble()-std::sqrt(700.0))<0.0001);
        QVERIFY(qAbs(metric.getOutputParam("StandardDeviation")->value.toDouble()-std::sqrt(600.0))<0.0001);
        QVERIFY(qAbs(metric.getOutputParam("ErrorPercent")->value.toDouble()-100.0/6.0)<0.0001);
        QCOMPARE(metric.getOutputParam("MaxError")->value.toInt(),90);
        QVERIFY(qAlpha(metric.getImageMask().pixel(1,1))>0);
        auto param=metric.getOutputParam("ErrorNum");
        metric.setDiscriminatingParam(param);
        param->threshold=0;
        QVERIFY(!metric.selectedStatsIsValid());
        param->threshold=1;
        QVERIFY(metric.selectedStatsIsValid());
    }
    void grayscaleOddWidth() {
        QImage left(QSize(5,3),QImage::Format_Grayscale8);
        left.fill(0);
        QImage right=left;
        right.scanLine(2)[4]=120;
        PerChannelMetric metric;
        metric.checkDifferences(write("gray-left.png",left),write("gray-right.png",right));
        QCOMPARE(metric.getPixelError(),1);
        QCOMPARE(metric.getOutputParam("MeanError")->value.toDouble(),8.0);
    }
    void mixedFormats() {
        QImage left=solid(qRgb(9,27,81));
        QImage right=left.convertToFormat(QImage::Format_RGB888);
        PerChannelMetric metric;
        metric.checkDifferences(write("mixed-left.png",left),write("mixed-right.bmp",right));
        QCOMPARE(metric.getPixelError(),0);
    }
    void premultipliedAndHighDepth() {
        const QImage straight=solid(qRgba(100,160,200,128));
        const QImage premult=straight.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        const QImage a=BaseMetric::comparisonImage(straight);
        const QImage b=BaseMetric::comparisonImage(premult);
        // Premultiplication quantizes channel values by at most one at this alpha.
        for(int x=0;x<a.width();++x) {
            QVERIFY(qAbs(qRed(a.pixel(x,0))-qRed(b.pixel(x,0)))<=1);
        }
        const QImage opaque=solid(qRgb(30,60,90));
        QCOMPARE(BaseMetric::comparisonImage(opaque.convertToFormat(QImage::Format_RGBA64)),BaseMetric::comparisonImage(opaque));
    }
    void transparentHiddenRgbIgnored() {
        PerChannelMetric metric;
        metric.checkDifferences(write("alpha-a.png",solid(qRgba(255,0,0,0))),write("alpha-b.png",solid(qRgba(0,0,255,0))));
        QCOMPARE(metric.getPixelError(),0);
    }
    void alphaChangesVisibleOnWhite() {
        PerChannelMetric metric;
        metric.checkDifferences(write("opaque.png",solid(qRgba(0,0,0,255))),write("transparent.png",solid(qRgba(0,0,0,0))));
        QCOMPARE(metric.getPixelError(),6);
        QCOMPARE(metric.getImageDifference().pixel(0,0),qRgb(255,255,255));
    }
    void meanUsesDifferenceOfMeans() {
        PerChannelMeanMetric metric;
        metric.checkDifferences(write("mean-red.png",solid(qRgb(255,0,0))),write("mean-blue.png",solid(qRgb(0,0,255))));
        QCOMPARE(metric.getPixelError(),0);
        metric.checkDifferences(write("mean-black.png",solid(qRgb(0,0,0))),write("mean-white.png",solid(qRgb(255,255,255))));
        QCOMPARE(metric.getOutputParam("MeanError")->value.toDouble(),255.0);
    }
    void weightedLuminance() {
        PerLuminanceMetric metric;
        metric.checkDifferences(write("lum-red.png",solid(qRgb(255,0,0))),write("lum-green.png",solid(qRgb(0,255,0))));
        QCOMPARE(metric.getImageDifference().pixel(0,0),qRgb(73,73,73));
        QCOMPARE(metric.getPixelError(),6);
    }
    void rejectDimensionsAndClearStaleResult() {
        PerChannelMetric metric;
        const auto left=write("size-a.png",solid(qRgb(0,0,0)));
        metric.checkDifferences(left,left);
        QVERIFY(metric.isValid());
        metric.checkDifferences(left,write("size-b.png",solid(qRgb(0,0,0),QSize(6,4))));
        QVERIFY(!metric.isValid());
        QVERIFY(metric.getImageDifference().isNull());
        QVERIFY(metric.getImageMask().isNull());
    }
    void invalidAndMissingInputs() {
        PerChannelMetric metric;
        QFile bad(files.filePath("invalid.png"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("not a PNG");
        bad.close();
        const auto good=write("good.png",solid(qRgb(0,0,0)));
        metric.checkDifferences(good,bad.fileName());
        QVERIFY(!metric.isValid());
        metric.checkDifferences(files.filePath("missing.png"),good);
        QVERIFY(!metric.isValid());
    }
    void outputDoesNotMutateInputs() {
        const auto left=write("immutable-a.png",solid(qRgb(10,20,30)));
        const auto right=write("immutable-b.png",solid(qRgb(40,50,60)));
        QFile f(left); QVERIFY(f.open(QIODevice::ReadOnly)); const auto before=f.readAll(); f.close();
        PerChannelMetric metric; metric.checkDifferences(left,right);
        const auto output=files.filePath("diff.png");
        QVERIFY(metric.saveDifference(output));
        QCOMPARE(QImage(output),metric.getImageDifference());
        QVERIFY(f.open(QIODevice::ReadOnly)); QCOMPARE(f.readAll(),before);
    }
    void perceptualIdenticalAndDifferent() {
        PerceptualMetric metric;
        const auto a=write("perceptual-a.png",solid(qRgb(0,0,0),QSize(32,32)));
        metric.checkDifferences(a,a);
        QVERIFY(metric.isValid()); QCOMPARE(metric.getPixelError(),0);
        metric.checkDifferences(a,write("perceptual-b.png",solid(qRgb(255,255,255),QSize(32,32))));
        QVERIFY(metric.isValid()); QVERIFY(metric.getPixelError()>0);
        metric.checkDifferences(a,a);
        QVERIFY(metric.isValid()); QCOMPARE(metric.getPixelError(),0);
    }
    void tinyBlueDifferenceHasMask() {
        PerChannelMetric metric;
        metric.checkDifferences(write("tiny-a.png",solid(qRgb(0,0,0))),write("tiny-b.png",solid(qRgb(0,0,1))));
        QCOMPARE(metric.getPixelError(),6);
        QCOMPARE(qAlpha(metric.getImageMask().pixel(0,0)),255);
    }
    void invalidPerceptualSettingsRejected() {
        PerceptualMetric metric;
        metric.getInputParams();
        metric.getInputParam("Gamma")->threshold = 0.0f;
        const auto file=write("invalid-gamma.png",solid(qRgb(0,0,0)));
        metric.checkDifferences(file,file);
        QVERIFY(!metric.isValid());
        QVERIFY(metric.getImageDifference().isNull());
    }
    void oversizedHeaderRejected() {
        QImage large(4097,4096,QImage::Format_Grayscale8);
        large.fill(0);
        PerChannelMetric metric;
        const auto file=write("oversized.png",large);
        metric.checkDifferences(file,file);
        QVERIFY(!metric.isValid());
        QVERIFY(metric.getImageDifference().isNull());
    }
    void guiComparisonSmoke() {
        AppSettings settings;
        settings.clear();
        Q_INIT_RESOURCE(diffimg);
        MetricsRegistering::registerAll();
        DiffImgWindow window;
        window.resize(1280,850);
        QImage left=solid(qRgb(220,220,220),QSize(400,300));
        QImage right=left;
        for(int y=100;y<180;++y) for(int x=130;x<250;++x) right.setPixel(x,y,qRgb(40,90,180));
        window.setFiles(write("gui-left.png",left),write("gui-right.png",right));
        window.show();
        window.load();
        QApplication::processEvents();
        QTest::qWait(100);
        auto statistics=window.findChild<QDockWidget *>("dockWidgetProperties");
        QVERIFY(statistics);
        QVERIFY(!statistics->isFloating());
        QVERIFY(statistics->width()>=260);
        QVERIFY(!window.findChildren<QGraphicsView *>().isEmpty());
        QVERIFY(window.windowTitle().contains("gui-left"));
        QVERIFY(window.grab().save("gui-smoke.png"));
        window.close();
    }
};
QTEST_MAIN(ComparisonTests)
#include "comparison_tests.moc"
