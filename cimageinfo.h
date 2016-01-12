#ifndef CIMAGEINFO_H
#define CIMAGEINFO_H

#include <QString>

class CImageInfo
{
public:
    CImageInfo(QString path);
    CImageInfo();
    virtual ~CImageInfo();

    QString getFullPath() const;
    void setFullPath(const QString &value);

    QString getBaseName() const;
    void setBaseName(const QString &value);

    int getSize() const;
    void setSize(int value);

    QString getFormattedSize() const;
    void setFormattedSize(const QString &value);

    bool isEqual(QString path);

    int getWidth() const;
    void setWidth(int value);

    int getHeight() const;
    void setHeight(int value);

private:
    QString fullPath;
    QString baseName;
    int size;
    QString formattedSize;
    int width;
    int height;
};

#endif // CIMAGEINFO_H
