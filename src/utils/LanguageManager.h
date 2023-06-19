#ifndef CAESIUM_IMAGE_COMPRESSOR_LANGUAGEMANAGER_H
#define CAESIUM_IMAGE_COMPRESSOR_LANGUAGEMANAGER_H

#include "Utils.h"
#include <QTranslator>
#include <vector>

class LanguageManager final {
public:
    LanguageManager() = delete;

    static QList<CsLocale> getTranslations();
    static QList<CsLocale> getSortedTranslations();
    static int findSortedIndex(const QString& languageId);
    static QString getLocaleFromPreferences(const QVariant& preference);
    static void loadLocale(QTranslator* translator);
};

#endif // CAESIUM_IMAGE_COMPRESSOR_LANGUAGEMANAGER_H
