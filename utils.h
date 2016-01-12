#ifndef UTILS_H
#define UTILS_H

#include "usageinfo.h"
#include <jpeglib.h>
#include <turbojpeg.h>

#include <QList>
#include <QStringList>
#include <QSize>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QTreeWidgetItem>

#define MAX_COLUMNS 7

enum cexifs {
    EXIF_COPYRIGHT,
    EXIF_DATE,
    EXIF_COMMENTS
};

enum list_columns {
    COLUMN_NAME = 0,
    COLUMN_ORIGINAL_SIZE = 1,
    COLUMN_NEW_SIZE = 2,
    COLUMN_ORIGINAL_RESOLUTION = 3,
    COLUMN_NEW_RESOLUTION = 4,
    COLUMN_SAVED = 5,
    COLUMN_PATH = 6
};

typedef struct cclt_compress_parameters {
    int quality;
    int width;
    int height;
    int scaling_factor;
    char* output_folder;
    int color_space;
    int dct_method;
    int exif_copy;
    int lossless;
    char** input_files;
    int input_files_count;
    int subsample;
    int recursive;
    int structure;
} cclt_compress_parameters;

enum image_type {
    JPEG,
    PNG,
    UNKN,
};

typedef struct var {
    int exif;
    QList<cexifs> importantExifs;
    int progressive;
    bool overwrite;
    int outMethodIndex;
    QString outMethodString;
} cparams;

extern QString inputFilter;
extern QString clfFilter;
extern QStringList inputFilterList;
extern QString versionString;
extern int versionNumber;
extern QString updateVersionTag;
extern int buildNumber;
extern long originalsSize, compressedSize; //Before and after bytes count
extern int compressedFiles; //Compressed files count
extern cparams params; //Important parameters
extern UsageInfo* uinfo;
extern QStringList osAndExtension;
extern QTemporaryDir tempDir;
extern QElapsedTimer timer;
extern QString lastCListPath; //Path of the last list saved

QString toHumanSize(int);
double humanToDouble(QString);
QString getRatio(qint64, qint64);
char* QStringToChar(QString s);
QSize getScaledSizeWithRatio(QSize size, int square); //Image preview resize
double ratioToDouble(QString ratio);
bool isJPEG(char* path);
QString msToFormattedString(qint64);
bool haveSameRootFolder(QList<QTreeWidgetItem *> items);
enum image_type detect_image_type(char* path);


#endif // UTILS_H
