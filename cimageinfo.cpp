#include "cimageinfo.h"
#include "utils.h"

#include <QFileInfo>
#include <QImageReader>

CImageInfo::CImageInfo(QString path) {
    QFileInfo* fi = new QFileInfo(path);
    fullPath = path;
    baseName = fi->completeBaseName();
    size = fi->size();
    formattedSize = toHumanSize(size);
    QImageReader* ir = new QImageReader(path);
    QSize s = ir->size();
    width = s.width();
    height = s.height();
}

CImageInfo::CImageInfo() {

}

CImageInfo::~CImageInfo() {

}

QString CImageInfo::getFullPath() const {
    return fullPath;
}

void CImageInfo::setFullPath(const QString &value) {
    fullPath = value;
}

QString CImageInfo::getBaseName() const {
    return baseName;
}

void CImageInfo::setBaseName(const QString &value) {
    baseName = value;
}

int CImageInfo::getSize() const {
    return size;
}

void CImageInfo::setSize(int value) {
    size = value;
}

QString CImageInfo::getFormattedSize() const {
    return formattedSize;
}

void CImageInfo::setFormattedSize(const QString &value) {
    formattedSize = value;
}

bool CImageInfo::isEqual(QString path) {
    return (QString::compare(fullPath, path) == 0);
}

int CImageInfo::getHeight() const {
    return height;
}

void CImageInfo::setHeight(int value) {
    height = value;
}

int CImageInfo::getWidth() const {
    return width;
}

void CImageInfo::setWidth(int value) {
    width = value;
}




