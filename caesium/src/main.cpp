/**
 *
 * This file is part of Caesium.
 *
 * Caesium - Caesium is an image compression software aimed at helping photographers,
 * bloggers, webmasters, businesses or casual users at storing, sending and sharing digital pictures.
 *
 * Copyright (C) 2016 - Matteo Paonessa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "caesium.h"
#include "utils.h"
#include "preferencedialog.h"
#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QLibraryInfo>
#include <QTranslator>
#include <QSettings>
#include <QStandardPaths>
//TODO Remove old updater.old

void logHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    //TODO Close the file?
    QByteArray localMsg = msg.toUtf8();

    QString logMessage;

    QFile *file = new QFile(logPath);

    if (file->open(QFile::Append | QIODevice::Text)) {
        QTextStream out(file);
        switch (type) {
        case QtDebugMsg:
            logMessage.sprintf("[%s] [DEBUG] %s \n(%s:%u, %s)\n",
                               QStringToChar(QTime::currentTime().toString("hh:mm:ss.zzz")),
                               localMsg.constData(),
                               context.file,
                               context.line,
                               context.function);
            out << logMessage;
            break;
        case QtInfoMsg:
            logMessage.sprintf("[%s] [INFO] %s\n",
                               QStringToChar(QTime::currentTime().toString("hh:mm:ss.zzz")),
                               localMsg.constData());
            out << logMessage;
            break;
        case QtWarningMsg:
            logMessage.sprintf("[%s] [WARNING] %s\n",
                               QStringToChar(QTime::currentTime().toString("hh:mm:ss.zzz")),
                               localMsg.constData());
            out << logMessage;
            break;
        case QtCriticalMsg:
            logMessage.sprintf("[%s] [CRITICAL] %s \n(%s:%u, %s)\n",
                               QStringToChar(QTime::currentTime().toString("hh:mm:ss.zzz")),
                               localMsg.constData(),
                               context.file,
                               context.line,
                               context.function);
            out << logMessage;
            break;
        case QtFatalMsg:
            logMessage.sprintf("[%s] [FATAL] %s \n(%s:%u, %s)\n",
                               QStringToChar(QTime::currentTime().toString("hh:mm:ss.zzz")),
                               localMsg.constData(),
                               context.file,
                               context.line,
                               context.function);
            out << logMessage;
            abort();

        out.flush();
        }
    } else {
        qCritical() << "Cannot log to file.";
    }
}

int main(int argc, char *argv[]) {
    //qInstallMessageHandler(logHandler);
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("Caesium");
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("saerasoft.com");

    QSettings settings;

    qInfo() << "----------------- Caesium session started at "
            << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << "-----------------";

    //Load languages
    loadLocales();
    QString locale;
    if (settings.value(KEY_PREF_GENERAL_LOCALE_STRING).value<QString>().isEmpty()) {
        locale = locales.at(0).name();
    } else {
        locale = settings.value(KEY_PREF_GENERAL_LOCALE_STRING).value<QString>();
    }

    //2x images for OSX Retina
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);

    //Load styles
#ifdef __APPLE__
    QFile qss(":/qss/style_osx.qss");
#elif _WIN32
    QFile qss(":/qss/style_win.qss");
#else
    QFile qss(":/qss/style_linux.qss");
#endif

    qss.open(QFile::ReadOnly);
    //Apply
    QString style(qss.readAll());
    a.setStyleSheet(style);

    //Translation support
    //Qt Widgets
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + locale,
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    //App translations
    QTranslator myappTranslator;
    bool tr_loaded = myappTranslator.load("caesium_" +
                                          locale,
                         QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&myappTranslator);

    Caesium w;
    w.show();

    qInfo() << "Translation path is: " << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    qInfo() << "Trying to load translation for language" << locale;
    qInfo() << "Translation loading result was" << tr_loaded;

    return a.exec();
}
