#include "LanguageManager.h"
#include <QApplication>
#include <QSettings>

QList<CsLocale> LanguageManager::getTranslations()
{
    QList<CsLocale> translations = {
        { QString("default"), QString("Default") },
        { QString("en_US"), QString("English (US)") },
        { QString("it_IT"), QString("Italiano") },
        { QString("fr_FR"), QString("Français") },
        { QString("hi_IN"), QString("हिन्दी") },
        { QString("ja_JP"), QString("日本語") },
        { QString("zh_CN"), QString("中文（中国大陆）") },
        { QString("es_ES"), QString("Español") },
        { QString("ar_EG"), QString("العَرَبِيَّة") },
        { QString("de_DE"), QString("Deutsch") },
        { QString("ru_RU"), QString("Русский") },
        { QString("tr_TR"), QString("Türkçe") },
        { QString("pt_BR"), QString("Português Brasileiro") },
        { QString("el_GR"), QString("Ελληνικά") },
        { QString("id_ID"), QString("Bahasa Indonesia") },
        { QString("zh_TW"), QString("中文（台灣）") },
        { QString("sk_SK"), QString("Slovenčina") },
        { QString("pl_PL"), QString("Polski") },
        { QString("fi_FI"), QString("Suomi") },
    };

    return translations;
}

QList<CsLocale> LanguageManager::getSortedTranslations()
{
    QList<CsLocale> sortedList = getTranslations();
    std::sort(sortedList.begin(), sortedList.end(), [](const CsLocale& a, const CsLocale& b) {
        if (a.label == "Default") {
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
    if (preference.typeId() == QVariant::Int || preference.typeId() == QVariant::LongLong) {
        int localeIndex = preference.toInt();
        return LanguageManager::getTranslations().at(localeIndex).locale;
    } else if (preference.typeId() == QVariant::String) {
        return preference.toString();
    } else {
        return "default";
    }
}

void LanguageManager::loadLocale(QTranslator* translator)
{
    QString localeId = LanguageManager::getLocaleFromPreferences(QSettings().value("preferences/language/locale", "default"));
    QLocale locale = QLocale();
    if (localeId != "default") {
        locale = QLocale(localeId);
    }
    if (translator->load(locale, QLatin1String("caesium"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        QCoreApplication::installTranslator(translator);
        QApplication::setLayoutDirection(locale.textDirection());
    }
}
