#include "lrmessagehub.h"

#include <QCoreApplication>
#include <QDebug>

namespace LimeReport {

MessageHub* MessageHub::instance()
{
    static MessageHub* hub = nullptr;
    if (!hub)
        hub = new MessageHub(QCoreApplication::instance());
    return hub;
}

void MessageHub::post(Severity severity, const QString& title, const QString& text)
{
    switch (severity) {
    case Information:
        qInfo().noquote() << title << ":" << text;
        break;
    case Warning:
        qWarning().noquote() << title << ":" << text;
        break;
    case Critical:
        qCritical().noquote() << title << ":" << text;
        break;
    }
    emit messagePosted(severity, title, text);
}

void MessageHub::information(QObject*, const QString& title, const QString& text)
{
    instance()->post(Information, title, text);
}

void MessageHub::warning(QObject*, const QString& title, const QString& text)
{
    instance()->post(Warning, title, text);
}

void MessageHub::critical(QObject*, const QString& title, const QString& text)
{
    instance()->post(Critical, title, text);
}

} // namespace LimeReport
