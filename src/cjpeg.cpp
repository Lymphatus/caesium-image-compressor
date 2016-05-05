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
