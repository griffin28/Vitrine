#include <QApplication>
#include <QProcessEnvironment>
#include <QString>

#include <iostream>

#include "AppMainWindow.h"
#include "AppUtils.h"

constexpr const char* KSETTINGSORG = "KSG-Technology-Consulting";
constexpr const char* KSETTINGDOMAIN = "ksgtechconsulting.com";
constexpr const char* KSETTINGSAPP = "Vitrine";

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

using AppUtils = vitrine::AppUtils;

int main(int argc, char *argv[])
{
    bool useDarkMode = true;
    QString anariLibrary;

    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]).toLower();
        if (arg == "--light") {
            useDarkMode = false;
        } else if (arg == "--dark") {
            useDarkMode = true;
        } else if (arg == "--anari-library") {
            if (i + 1 < argc) {
                anariLibrary = QString::fromLocal8Bit(argv[++i]);
            } else {
                std::cerr << "--anari-library requires a value." << std::endl;
            }
        }
    }

    if (anariLibrary.isEmpty()) {
        const auto env = QProcessEnvironment::systemEnvironment();
        anariLibrary = env.value(QStringLiteral("ANARI_LIBRARY"));
    }

    QApplication app(argc, argv);
    if (useDarkMode) {
        AppUtils::applyDarkMode(app);
    } else {
        AppUtils::applyLightMode(app);
    }

    QApplication::setOrganizationName(KSETTINGSORG);
    QApplication::setOrganizationDomain(KSETTINGDOMAIN);
    QApplication::setApplicationName(KSETTINGSAPP);

    vitrine::AppMainWindow mainWindow(anariLibrary, useDarkMode);
    mainWindow.resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    mainWindow.show();

    return app.exec();
}
