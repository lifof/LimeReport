
# LimeReport v2.0.0 (Qt Quick) ![Cmake Build Status](https://github.com/fralx/limereport/actions/workflows/cmake.yml/badge.svg)

## Official LimeReport web site [http://limereport.ru](http://limereport.ru)

## Features

* Multi-platform support
* Pure Qt 6 Qt Quick code: no QtWidgets, no QtPrintSupport
* Embedded report designer and preview built with QtQuick.Controls
* Various band types for a report of any complexity
* Page header and footer
* Data grouping (GroupHeader, GroupFooter, Subdetail, SubdetailHeader, SubdetailFooter)
* Aggregation functions (SUM, COUNT, AVG, MIN, MAX)
* Report elements: Text, Geometric (Line, Ellipsis, Rectangle), Picture, SVG, Chart, Barcode
* Horizontal and vertical element groups
* HTML to format input fields
* Scripts to format output data
* An Automatic band height adjustment
* A Smart band split moving data to the next page
* PDF output (QPdfWriter) and rendering to images

### Qt Quick edition

This branch runs entirely on Qt Quick. The QtWidgets designer and preview have
been replaced by a QML module (`import LimeReport`) built with QtQuick.Controls:

* The report engine no longer depends on QGraphicsScene: items are drawn by a
  small QtGui-only scene graph (`limereport/lrgraphicsscene.h`), so reports can
  be rendered headless or inside any Qt Quick scene.
* `ReportPreview` / `ReportPreviewController`: page navigation, zoom, edit mode,
  PDF and exporter output, saving and loading prepared pages.
* `ReportDesigner` / `ReportDesignerController`: pages, bands, item insertion,
  undo/redo, clipboard, alignment, layouts, fonts, borders, script editor,
  object tree, data browser (connections, SQL/CSV datasources, variables with
  drag and drop onto the page) and a property inspector.
* `ReportSceneView`: a `QQuickPaintedItem` that displays a report scene.
* `ReportEngine` can be created from QML; engine messages are available through
  the `ReportMessages` singleton.

Requirements: Qt 6.4 or later with the Core, Gui, Qml, Quick, QuickControls2,
Sql, Xml and Svg modules, and CMake 3.16 or later.

Not carried over from the widget edition: printing through QPrinter and the
print dialog (`printReport()` renders a PDF and hands it to the platform viewer),
the Qt Designer plugin, the embedded dialog designer (report dialogs built from
`.ui` files), the translation editor and the chart series editor dialog (chart
properties are still available in the property inspector). The qmake project
files are gone; the library is built with CMake.

### How to build

```sh
cmake -S . -B build -DENABLE_ZINT=ON
cmake --build build
ctest --test-dir build   # runs offscreen
```

Options: `LIMEREPORT_STATIC`, `ENABLE_ZINT`, `LIMEREPORT_BUILD_DESIGNER`,
`LIMEREPORT_BUILD_DEMO`, `LIMEREPORT_BUILD_TESTS`.

### How to use it

To use it in your CMake project without installing it, either add it as a
subdirectory:

```cmake
add_subdirectory(LimeReport)
target_link_libraries(myapp PRIVATE limereport-qt6)
```

or fetch it:

```cmake
include(FetchContent)
FetchContent_Declare(
  LimeReport
  GIT_REPOSITORY https://github.com/fralx/LimeReport.git
  GIT_TAG        sha-of-the-commit
)
FetchContent_MakeAvailable(LimeReport)
target_link_libraries(myapp PRIVATE limereport-qt6)
```

From C++ (a `QGuiApplication` is enough):

```cpp
  #include <LimeReport>              // report engine
  #include <LRCallbackDS>            // if you want use callback datasources

  report = new LimeReport::ReportEngine(this);
  report->dataManager()->addModel("string_list", stringListModel, true);
  report->loadFromFile("File name");
  report->previewReport();           // Qt Quick preview window
  report->designReport();            // Qt Quick designer window
  report->printToPDF("report.pdf");
  QList<QImage> pages = report->renderToImages(150);
```

From QML (link the application against the library; with Qt 6.4 add the
`qrc:/qt/qml` import path to the QML engine):

```qml
import QtQuick
import QtQuick.Controls
import LimeReport

ApplicationWindow {
    visible: true
    ReportEngine { id: engine }

    // an embedded designer
    ReportDesigner { anchors.fill: parent; engine: engine }

    // or an embedded preview
    // ReportPreview {
    //     anchors.fill: parent
    //     controller: ReportPreviewController { id: preview; engine: engine }
    //     Component.onCompleted: if (engine.loadFromUrl(reportUrl)) preview.render()
    // }
}
```

With a static build call `LimeReport::registerQmlTypes()` before loading QML
that uses the module if no `ReportEngine` has been created yet.

For more samples see the demo (`demo_r1`).

### Change log

#### 2.0.0

1. The library runs on Qt Quick only: QtWidgets and QtPrintSupport are no longer used.
2. New QtQuick.Controls report designer and preview (QML module `LimeReport`).
3. QGraphicsScene replaced by a QtGui-only scene graph.
4. PDF output uses QPdfWriter; `renderToImages()` added.
5. Qt 6 and CMake are required.

#### 1.5.0

1. Added the ability to use QJSEngine instead of deprecated QtScript.
2. Report designer has been improved.
3. Inches support has been added.
4. Embedded dialog designer has been added.
5. The script editor has been improved.
6. Added the ability to build only report generator without embedded visual report designer.
7. Report translation ability has been added.
8. Added report generation time events with the ability to process them in the report script.
9. Added the ability to build a report table of contents.
10. The vertical layout has been added.
11. Added the ability to transfer an image to the report via a variable.
12. Endless height has been added.
13. Added the ability to print a report page on multiple pages of paper.
14. Added the ability to print on multiple printers.
15. ChartItem has been added.
16. Added the ability to use aggregate functions in headers.
17. Subtotals.
18. Dark and Light themes have been added to report designer.
19. Generation result editing mode has been improved.
20. And many other minor fixes and improvements.

#### 1.4.7

1. Multipage.
2. Dialogs.
3. Render events.
4. Initscript.
5. Memory usage has been reduced.
6. Data source manager has been refactored.
7. Report items context menus have been added.
8. Editable report.
9. And many other minor fixes and improvements.

#### 1.3.11

1. The LimeReport project structure has been changed.
2. Preview widget has been added.
3. A new demo has been added.
4. Landscape page orientation has been fixed.
5. Other minor bugs have been fixed.

#### 1.3.10

1. A memory leak has been fixed.
2. Grid & Settings have been added.
3. Recent files menu has been added.
4. Magnet feature has been added.
5. Added ability to use variables in the connection settings.

#### 1.3.9

New functions:

```cpp
  QString::saveToString(),
  loadFromString(const QString& report, const QString& name=""),
  QByteArray::saveToByteArray(),
  setCurrentReportsDir(const QString& dirName),
```

added to LimeReport::ReportEngine

1. printOnEach page and columns have been added to DataHeader band
2. startNewPage added to DataBand

Performance has been improved

**WARNING**
From this version, the item "Text" by default does not use HTML.
To enable HTML support you need to use the property allowHTML

#### 1.3.1

Added:

1. Columns
   Some bands can be divided into columns
2. Items align
   Report items now may be aligned to the left, right or center of the container
   also it can be stretched to the whole width of the container
3. Group can start a new page
4. Group can reset page number;
5. Table mode added to the horizontal layout
   This mode allows you to distribute the internal layout's space among grouped items

Fixed:

1. Postgresql connection
2. The error that prevented the normal run of more than one instance of LimeReport::ReportEngine

#### 1.2.1

1. Added buttons to open / hide sidebars;
2. Improved look-and-feel of report elements to clarify the designing process;
3. Printing to PDF added.  
4. Fixed bug in SQL-editor when it used variables in SQL expression;
5. Fixed bug of variable's initialization if it exists more than once in SQL expression;
6. .. and other minor bugs fixed.
