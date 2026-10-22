
/******************************************************************************
   DiffImg: image difference viewer
   Copyright(C) 2011-2014  xbee@xbee.net

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *******************************************************************************/

#include <QtCore/QDebug>

#include "PerLuminanceMetric.h"

#include "MiscFunctions.h"
#include "LogHandler.h"

PerLuminanceMetric::PerLuminanceMetric(QObject *parent) : BaseMetric(parent)
{
    m_type = metaObject()->className();
    m_name = tr("Difference per image luminance");
    m_desc = tr("Difference is computed between both luminance values defined as luminance(x,y) = 0.3*R(x,y)+0.59*G(x,y)+0.11*B(x,y)");

    // init settings
    //loadSettings();
}

PerLuminanceMetric::~PerLuminanceMetric()
{
}

QPixmap PerLuminanceMetric::getLogo() const
{
    return QPixmap(":/diffimg/perluminance.png");
}

#define Q_MAX_3(a, b, c) ( ( a > b && a > c) ? a : (b > c ? b : c) )
#define Q_MIN_3(a, b, c) ( ( a < b && a < c) ? a : (b < c ? b : c) )

// from HSL
static inline int getLightness(const QRgb rgb)
{
    const int r = qRed(rgb);
    const int g = qGreen(rgb);
    const int b = qBlue(rgb);
    const int max = Q_MAX_3(r, g, b);
    const int min = Q_MIN_3(r, g, b);
    return qreal(0.5) * (max + min);
}

void PerLuminanceMetric::performDifference()
{
    const QImage a = comparisonImage(m_image1);
    const QImage b = comparisonImage(m_image2);
    QImage output(a.size(), QImage::Format_RGB32);
    auto luminance = [](QRgb pixel) {
        return (30*qRed(pixel)+59*qGreen(pixel)+11*qBlue(pixel)+50)/100;
    };
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            const int difference = qAbs(luminance(a.pixel(x,y))-luminance(b.pixel(x,y)));
            output.setPixel(x,y,qRgb(difference,difference,difference));
        }
    }
    m_imageDiff = output;
}
