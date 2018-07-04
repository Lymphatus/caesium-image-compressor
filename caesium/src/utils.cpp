/*
 *
 * This file is part of Caesium Image Compressor.
 *
 * Caesium Image Compressor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Caesium Image Compressor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Caesium Image Compressor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <src/utils.h>

#include <math.h>
#include <QStringList>
#include <QDebug>

QString toHumanSize(size_t size)
{
    //Check if size is 0 to avoid crashes
    if (size == 0) {
        return "0 bytes";
    }

    QStringList unit;
    unit << "bytes" << "Kb" << "Mb" << "Gb" << "Tb";
    //Index of the array containing the correct unit
    double order = floor(log2(labs(size)) / 10);

    //We should never handle files over 1k Tb, but...
    if (order > 4) {
        qWarning() << "Woah, that's huge!";
        order = 4;
    }

    return QString::number(size / (pow(1024, order)), 'f', 2) + ' ' + unit[(int)order];
}

QSize getScaledSizeWithRatio(QSize size, int square) {
    int w = size.width();
    int h = size.height();

    double ratio = 0.0;

    //Check the biggest between the two and scale on that dimension
    if (w >= h) {
        ratio = w / (double) square;
    } else {
        ratio = h / (double) square;
    }

    return QSize((int) round(w / ratio), (int) h / ratio);
}
