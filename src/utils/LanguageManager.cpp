#include "LanguageManager.h"
#include <QDebug>

QList<CsLocale> LanguageManager::getTranslations()
{
    QList<CsLocale> translations = {
        { QString("default"), QString("Default") },
        { QString("en_US"), QString("English (US)") },
        { QString("it_IT"), QString("Italiano") },
        { QString("fr_FR"), QString("Français") },
        { QString("ja_JP"), QString("日本語") },
        { QString("zh_CN"), QString("简体中文") },
        { QString("es_ES"), QString("Español") },
        { QString("ar_EG"), QString("العَرَبِيَّة") },
        { QString("de_DE"), QString("Deutsch") },
        { QString("ru_RU"), QString("Русский") },
        { QString("tr_TR"), QString("Türkçe") },
        { QString("pt_BR"), QString("Português Brasileiro") },
        { QString("el_GR"), QString("ελληνική γλώσσα") },
    };

    return translations;
}

QList<CsLocale> LanguageManager::getSortedTranslations()
{
    QList<CsLocale> sortedList = getTranslations();
    std::sort(sortedList.begin(), sortedList.end(), [](const CsLocale& a, const CsLocale& b) {
        if (a.label == "default") {
            return true;
        }
        return a.label < b.label;
    });
    return sortedList;
}

int LanguageManager::findSortedIndex(const QString& languageId)
{
    QList<CsLocale> sortedList = LanguageManager::getSortedTranslations();
    int index = 0;
    for (const CsLocale& locale : sortedList) {
        if (locale.locale == languageId) {
            return index;
        }
        index++;
    }
    return index;
}

QString LanguageManager::getLocaleFromPreferences(const QVariant& preference)
{
    QString locale = "default";
    if (preference.type() == QVariant::Int || preference.type() == QVariant::LongLong) {
        int localeIndex = preference.toInt();
        locale = LanguageManager::getTranslations().at(localeIndex).locale;
    } else if (preference.type() == QVariant::String) {
        locale = preference.toString();
    }

    return locale;
}
