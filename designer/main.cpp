#include "designersettingmanager.h"

#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QQuickStyle>
#include <QTranslator>

#include <LimeReport>

int main(int argc, char* argv[])
{
    QGuiApplication a(argc, argv);
    a.setApplicationName("LRDesigner");
    a.setOrganizationName("LimeReport");
    a.setWindowIcon(QIcon(":/report/images/logo32"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle("Fusion");

    DesignerSettingManager manager;

    QTranslator limeReportTranslator;
    QTranslator qtBaseTranslator;

    QString translationPath = QCoreApplication::applicationDirPath();
    translationPath.append("/translations");
    Qt::LayoutDirection layoutDirection = QLocale::system().textDirection();

    QString designerTranslation = QLocale(manager.getCurrentDefaultLanguage()).name();

    if (limeReportTranslator.load("limereport_" + designerTranslation, translationPath)) {
        if (qtBaseTranslator.load("qtbase_" + designerTranslation, translationPath))
            a.installTranslator(&qtBaseTranslator);
        a.installTranslator(&limeReportTranslator);
        layoutDirection = QLocale(manager.getCurrentDefaultLanguage()).textDirection();
        a.setLayoutDirection(layoutDirection);
    }

    LimeReport::ReportEngine report;
    report.setPreviewLayoutDirection(layoutDirection);

    if (a.arguments().count() > 1) {
        report.loadFromFile(a.arguments().at(1));
    }
    QObject::connect(&report, SIGNAL(getAvailableDesignerLanguages(QList<QLocale::Language>*)),
                     &manager, SLOT(getAvailableLanguages(QList<QLocale::Language>*)));

    QObject::connect(&report, SIGNAL(getCurrentDefaultDesignerLanguage()), &manager,
                     SLOT(getCurrentDefaultLanguage()));

    QObject::connect(&report, SIGNAL(currentDefaultDesignerLanguageChanged(QLocale::Language)),
                     &manager, SLOT(currentDefaultLanguageChanged(QLocale::Language)));

    // The designer window runs its own event loop and returns when it is closed.
    report.setShowDesignerModal(true);
    report.designReport();
    return 0;
}
