#pragma once
// GPL-2.0-or-later. DiffImg Windows revival, 2026.
#include <QImage>
#include <atomic>
#include <QJsonObject>
#include <QMap>
#include <QRect>
#include <QStringList>
namespace Review {
struct Profile {
    QString name="Default";
    QString metric="PerChannelMetric";
    int allowedPixels=0, maxError=255;
    double meanError=255;
    QMap<QString,double> parameters;
    QMap<QString,QList<QRect>> masks;
    void validate() const;
    QJsonObject json() const;
    static Profile fromJson(const QJsonObject &);
    static Profile load(const QString &);
    void save(const QString &) const;
};
struct Entry {
    QString relative,left,right,status,detail;
    QByteArray leftHash,rightHash;
    int pixels=0,maxError=0;
    double meanError=0;
    QSize size;
    QList<QRect> regions;
    bool comparable() const {return status=="Identical" || status=="Accepted" || status=="Changed";}
};
QByteArray hashFile(const QString &);
QImage readImage(const QString &);
QList<Entry> discover(const QString &baseline,const QString &current,const std::atomic_bool *cancel=nullptr);
Entry compare(Entry,const Profile &,QImage *difference=nullptr);
void exportHtml(const QString &newFile,const QList<Entry> &,const Profile &);
QString approve(const QString &baseline,const QString &current,const QList<Entry> &selected);
void restore(const QString &baseline,const QString &manifest);
int batch(const QStringList &arguments);
}
