#ifndef CJPEG_H
#define CJPEG_H

#include "src/cimage.h"
#include "src/utils.h"
#include <turbojpeg.h>

class CJPEG : public CImage
{

public:
    using CImage::CImage;

    int getQuality() const;
    void setQuality(int value);

    int getColor_space() const;
    void setColor_space(int value);

    int getDct_method() const;
    void setDct_method(int value);

    bool getExif() const;
    void setExif(bool value);

    QList<cexifs> getImportantExifs() const;
    void setImportantExifs(const QList<cexifs> &value);

    TJSAMP getSubsample() const;
    void setSubsample(const TJSAMP &value);

    bool getProgressive() const;
    void setProgressive(bool value);
    
    QString toString();

private:
    int quality = 65;
    int color_space = TJCS_RGB;
    int dct_method = TJFLAG_FASTDCT;
    bool exif = true;
    QList<cexifs> importantExifs = {};
    enum TJSAMP subsample;
    bool progressive = true;
};

#endif // CJPEG_H
