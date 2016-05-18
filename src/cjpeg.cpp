#include "cjpeg.h"


int CJPEG::getQuality() const
{
    return quality;
}

void CJPEG::setQuality(int value)
{
    quality = value;
}

int CJPEG::getColor_space() const
{
    return color_space;
}

void CJPEG::setColor_space(int value)
{
    color_space = value;
}

int CJPEG::getDct_method() const
{
    return dct_method;
}

void CJPEG::setDct_method(int value)
{
    dct_method = value;
}

bool CJPEG::getExif() const
{
    return exif;
}

void CJPEG::setExif(bool value)
{
    exif = value;
}

QList<cexifs> CJPEG::getImportantExifs() const
{
    return importantExifs;
}

void CJPEG::setImportantExifs(const QList<cexifs> &value)
{
    importantExifs = value;
}

TJSAMP CJPEG::getSubsample() const
{
    return subsample;
}

void CJPEG::setSubsample(const TJSAMP &value)
{
    subsample = value;
}

bool CJPEG::getProgressive() const
{
    return progressive;
}

void CJPEG::setProgressive(bool value)
{
    progressive = value;
}

QString CJPEG::toString() {
    QString final = "[";
    
    if (quality == 0) {
        final.append(QObject::tr("Lossless") + ", ");
    } else {
        final.append(QObject::tr("Quality: ") + QString::number(quality) + ", ");
    }
    
    final.append(QObject::tr("progressive") + ", ");
    if (exif && importantExifs.length() == 3) { //All exifs
        final.append(QObject::tr("exif: all"));
    } else if (exif && !importantExifs.isEmpty()) { //Some exifs
        final.append(QObject::tr("exif: "));
        for (int i = 0; i < importantExifs.length() - 1; i++) {
            final.append(importantExifs.at(i));
        }
        final.append(importantExifs.last());
    }
    final.append("]");
    
    return final;
}
