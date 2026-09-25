#ifndef LRQUICKWINDOWS_H
#define LRQUICKWINDOWS_H

#include <QObject>
#include <functional>
#include "lrglobal.h"
#include "lrreportengine.h"

namespace LimeReport {

/*
 * Opens the Qt Quick preview / designer windows that replace the former
 * QMainWindow based PreviewReportWindow and ReportDesignWindow.
 * A QGuiApplication (or QApplication) must exist.
 */
namespace QuickWindows {
    typedef std::function<void(QObject* window)> WindowCreated;
    QObject* showPreview(ReportEngine* engine, ReportPages pages, PreviewHints hints, bool modal,
                         WindowCreated onCreated = WindowCreated());
    QObject* showDesigner(ReportEngine* engine, bool modal, WindowCreated onCreated = WindowCreated());
    void closeWindow(QObject* window);
}

} // namespace LimeReport

#endif // LRQUICKWINDOWS_H
