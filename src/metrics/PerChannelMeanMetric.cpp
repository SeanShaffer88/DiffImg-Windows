
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

#include "PerChannelMeanMetric.h"

#include "MiscFunctions.h"
#include "LogHandler.h"

PerChannelMeanMetric::PerChannelMeanMetric(QObject *parent) : BaseMetric(parent)
{
    m_type = metaObject()->className();
    m_name = tr("Difference per channel mean");
    m_desc = tr("Difference is computed between both mean values defined as mean(x,y) = (R(x,y)+G(x,y)+B(x,y))/3");

    // init settings
    //loadSettings();
}

PerChannelMeanMetric::~PerChannelMeanMetric()
{
}

QPixmap PerChannelMeanMetric::getLogo() const
{
    return QPixmap(":/diffimg/perchannelmean.png");
}

void PerChannelMeanMetric::performDifference()
{
    const QImage a = comparisonImage(m_image1);
    const QImage b = comparisonImage(m_image2);
    QImage output(a.size(), QImage::Format_RGB32);
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            const QRgb left = a.pixel(x,y), right = b.pixel(x,y);
            const int sum1 = qRed(left)+qGreen(left)+qBlue(left);
            const int sum2 = qRed(right)+qGreen(right)+qBlue(right);
            const int difference = qAbs(sum1-sum2)/3;
            output.setPixel(x,y,qRgb(difference,difference,difference));
        }
    }
    m_imageDiff = output;
}
