#include "lrquickwindows.h"

#include "lrquickdesigner.h"
#include "lrquickpreview.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QGuiApplication>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>

extern void qml_register_types_LimeReport();

namespace LimeReport {

void registerQmlTypes()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;
#ifdef HAVE_STATIC_BUILD
    // In static builds nothing references the generated registration unit,
    // so the linker would drop it: register explicitly.
    qml_register_types_LimeReport();
#endif
}

namespace QuickWindows {

    static QQmlEngine* sharedEngine()
    {
        registerQmlTypes();
        static QPointer<QQmlEngine> engine;
        if (!engine) {
            engine = new QQmlEngine(QCoreApplication::instance());
            engine->addImportPath(QStringLiteral("qrc:/qt/qml"));
        }
        return engine;
    }

    static QQuickWindow* createWindow(const QString& file, const QVariantMap& properties)
    {
        if (!qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
            qWarning() << "LimeReport: a QGuiApplication is required to show" << file;
            return nullptr;
        }
        QQmlEngine* engine = sharedEngine();
        QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/qt/qml/LimeReport/") + file));
        if (component.isError()) {
            qWarning() << "LimeReport:" << component.errors();
            return nullptr;
        }
        QObject* object = component.createWithInitialProperties(properties);
        if (!object) {
            qWarning() << "LimeReport:" << component.errors();
            return nullptr;
        }
        QQuickWindow* window = qobject_cast<QQuickWindow*>(object);
        if (!window) {
            delete object;
            return nullptr;
        }
        QQmlEngine::setObjectOwnership(window, QQmlEngine::CppOwnership);
        return window;
    }

    static void runWindow(QQuickWindow* window, bool modal)
    {
        QPointer<QQuickWindow> guard(window);
        if (modal) {
            window->setModality(Qt::ApplicationModal);
            QEventLoop loop;
            QObject::connect(window, &QWindow::visibleChanged, &loop, [&loop](bool visible) {
                if (!visible)
                    loop.quit();
            });
            QObject::connect(window, &QObject::destroyed, &loop, &QEventLoop::quit);
            window->show();
            loop.exec();
            if (guard)
                guard->deleteLater();
        } else {
            QObject::connect(window, &QWindow::visibleChanged, window, [window](bool visible) {
                if (!visible)
                    window->deleteLater();
            });
            window->show();
        }
    }

    QObject* showPreview(ReportEngine* engine, ReportPages pages, PreviewHints hints, bool modal,
                         WindowCreated onCreated)
    {
        QuickReportPreview* controller = new QuickReportPreview();
        controller->setEngineObject(engine);
        controller->setPages(pages);
        QVariantMap properties;
        properties.insert("controller", QVariant::fromValue<QObject*>(controller));
        properties.insert("hints", int(hints));
        QQuickWindow* window = createWindow(QStringLiteral("PreviewWindow.qml"), properties);
        if (!window) {
            delete controller;
            return nullptr;
        }
        controller->setParent(window);
        if (onCreated)
            onCreated(window);
        QPointer<QQuickWindow> guard(window);
        runWindow(window, modal);
        return modal ? nullptr : guard.data();
    }

    QObject* showDesigner(ReportEngine* engine, bool modal, WindowCreated onCreated)
    {
        QVariantMap properties;
        properties.insert("engine", QVariant::fromValue<QObject*>(engine));
        QQuickWindow* window = createWindow(QStringLiteral("DesignerWindow.qml"), properties);
        if (!window)
            return nullptr;
        if (onCreated)
            onCreated(window);
        QPointer<QQuickWindow> guard(window);
        runWindow(window, modal);
        return modal ? nullptr : guard.data();
    }

    void closeWindow(QObject* window)
    {
        if (QWindow* w = qobject_cast<QWindow*>(window))
            w->close();
    }

} // namespace QuickWindows
} // namespace LimeReport
