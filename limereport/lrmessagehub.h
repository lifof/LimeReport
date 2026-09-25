#ifndef LRMESSAGEHUB_H
#define LRMESSAGEHUB_H

#include "lrglobal.h"

#include <QObject>
#include <QString>

namespace LimeReport {

/*
 * Replaces the modal QMessageBox calls of the widget based implementation.
 * The engine posts user facing messages here; a UI (for example the QML
 * designer or preview) listens to messagePosted() and shows them.
 * Messages are also forwarded to the Qt message log.
 */
class LIMEREPORT_EXPORT MessageHub: public QObject {
    Q_OBJECT
public:
    enum Severity {
        Information,
        Warning,
        Critical
    };
    Q_ENUM(Severity)
    static MessageHub* instance();
    static void information(QObject* parent, const QString& title, const QString& text);
    static void warning(QObject* parent, const QString& title, const QString& text);
    static void critical(QObject* parent, const QString& title, const QString& text);
    void post(Severity severity, const QString& title, const QString& text);
signals:
    void messagePosted(int severity, const QString& title, const QString& text);

private:
    explicit MessageHub(QObject* parent = nullptr): QObject(parent) { }
};

} // namespace LimeReport

#endif // LRMESSAGEHUB_H
