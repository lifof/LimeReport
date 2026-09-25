#ifndef LRQUICKREGISTRATION_H
#define LRQUICKREGISTRATION_H

#include "lrmessagehub.h"
#include "lrreportengine.h"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

namespace LimeReport {

// Makes LimeReport::ReportEngine creatable from QML as "ReportEngine".
struct QuickForeignReportEngine {
    Q_GADGET
    QML_FOREIGN(LimeReport::ReportEngine)
    QML_NAMED_ELEMENT(ReportEngine)
};

// Engine notifications (errors, warnings) as a QML singleton "ReportMessages".
struct QuickForeignMessageHub {
    Q_GADGET
    QML_FOREIGN(LimeReport::MessageHub)
    QML_NAMED_ELEMENT(ReportMessages)
    QML_SINGLETON
public:
    static MessageHub* create(QQmlEngine*, QJSEngine*)
    {
        MessageHub* hub = MessageHub::instance();
        QJSEngine::setObjectOwnership(hub, QJSEngine::CppOwnership);
        return hub;
    }
};

} // namespace LimeReport

#endif // LRQUICKREGISTRATION_H
