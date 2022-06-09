#ifndef CAESIUM_IMAGE_COMPRESSOR_CIMAGELIST_H
#define CAESIUM_IMAGE_COMPRESSOR_CIMAGELIST_H

#include "CImage.h"
#include <QList>
#include <QStringList>

class CImageList {
public:
    CImageList();
    ~CImageList();

    explicit CImageList(const QStringList& fileList);
    explicit CImageList(const QList<CImage>& cImageList);

    void append(const QStringList& fileList);
    void append(const QList<CImage>& cImageList);
    void append(const QString& filePath);

private:
    QList<CImage>* cImageList;
};

#endif // CAESIUM_IMAGE_COMPRESSOR_CIMAGELIST_H
