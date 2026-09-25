cmake_minimum_required(VERSION 3.16)
project(limereport LANGUAGES CXX C)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" "${CMAKE_MODULE_PATH}")
include(GenerateVersionHeader)

set(DEFAULT_ITEM_PADDING 0 CACHE STRING "Default padding of report items")

option(ENABLE_ZINT "Enable libzint build for barcode support" OFF)
option(LIMEREPORT_STATIC "Build LimeReport as static library" OFF)
option(LIMEREPORT_BUILD_DESIGNER "Build the LRDesigner application" ON)
option(LIMEREPORT_BUILD_DEMO "Build the demo application" ON)
option(LIMEREPORT_BUILD_TESTS "Build the tests" ON)

# LimeReport is a pure Qt Quick library: no QtWidgets, no QtPrintSupport.
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Sql Xml Svg Qml Quick QuickControls2)
set(QT_VERSION_MAJOR 6)
qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

add_subdirectory(3rdparty)

set(LR limereport)

set(LIMEREPORT_SOURCES
    ${LR}/bands/lrdataband.cpp
    ${LR}/bands/lrgroupbands.cpp
    ${LR}/bands/lrpagefooter.cpp
    ${LR}/bands/lrpageheader.cpp
    ${LR}/bands/lrreportfooter.cpp
    ${LR}/bands/lrreportheader.cpp
    ${LR}/bands/lrsubdetailband.cpp
    ${LR}/bands/lrtearoffband.cpp
    ${LR}/exporters/lrpdfexporter.cpp
    ${LR}/items/charts/lrhorizontalbarchart.cpp
    ${LR}/items/charts/lrlineschart.cpp
    ${LR}/items/charts/lrgridlineschart.cpp
    ${LR}/items/charts/lrpiechart.cpp
    ${LR}/items/charts/lrverticalbarchart.cpp
    ${LR}/items/lrabstractlayout.cpp
    ${LR}/items/lrchartitem.cpp
    ${LR}/items/lrhorizontallayout.cpp
    ${LR}/items/lrimageitem.cpp
    ${LR}/items/lrlayoutmarker.cpp
    ${LR}/items/lrshapeitem.cpp
    ${LR}/items/lrsvgitem.cpp
    ${LR}/items/lrtextitem.cpp
    ${LR}/items/lrverticallayout.cpp
    ${LR}/lraxisdata.cpp
    ${LR}/lrbanddesignintf.cpp
    ${LR}/lrbandsmanager.cpp
    ${LR}/lrbasedesignintf.cpp
    ${LR}/lrdatadesignintf.cpp
    ${LR}/lrdatasourcemanager.cpp
    ${LR}/lrglobal.cpp
    ${LR}/lrmessagehub.cpp
    ${LR}/lrgraphicsscene.cpp
    ${LR}/lrgroupfunctions.cpp
    ${LR}/lritemdesignintf.cpp
    ${LR}/lritemscontainerdesignitf.cpp
    ${LR}/lrpagedesignintf.cpp
    ${LR}/lrpageitemdesignintf.cpp
    ${LR}/lrpreparedpages.cpp
    ${LR}/lrreportengine.cpp
    ${LR}/lrreportrender.cpp
    ${LR}/lrreporttranslation.cpp
    ${LR}/lrscriptenginemanager.cpp
    ${LR}/lrsimplecrypt.cpp
    ${LR}/lrvariablesholder.cpp
    ${LR}/serializators/lrxmlbasetypesserializators.cpp
    ${LR}/serializators/lrxmlqrectserializator.cpp
    ${LR}/serializators/lrxmlreader.cpp
    ${LR}/serializators/lrxmlwriter.cpp
    ${LR}/quick/lrquicksceneview.cpp
    ${LR}/quick/lrquickpreview.cpp
    ${LR}/quick/lrquickdesigner.cpp
    ${LR}/quick/lrquickpropertymodel.cpp
    ${LR}/quick/lrquickdatamodel.cpp
    ${LR}/quick/lrquickwindows.cpp
)

file(GLOB_RECURSE LIMEREPORT_HEADERS ${LR}/*.h)
if (NOT ENABLE_ZINT)
    list(FILTER LIMEREPORT_HEADERS EXCLUDE REGEX "lrbarcodeitem\\.h$")
endif()
if (NOT LIMEREPORT_STATIC)
    list(FILTER LIMEREPORT_HEADERS EXCLUDE REGEX "lrfactoryinitializer\\.h$")
endif()

set(LIMEREPORT_RESOURCES
    ${LR}/items/items.qrc
    ${LR}/report.qrc
)

if (ENABLE_ZINT)
    list(APPEND LIMEREPORT_SOURCES ${LR}/items/lrbarcodeitem.cpp)
endif()

if (LIMEREPORT_STATIC)
    list(APPEND LIMEREPORT_SOURCES ${LR}/lrfactoryinitializer.cpp)
endif()

set(LIMEREPORT_TARGET limereport-qt6)

if (LIMEREPORT_STATIC)
    add_library(${LIMEREPORT_TARGET} STATIC)
    target_compile_definitions(${LIMEREPORT_TARGET} PUBLIC HAVE_STATIC_BUILD)
else()
    add_library(${LIMEREPORT_TARGET} SHARED)
    target_compile_definitions(${LIMEREPORT_TARGET} PRIVATE LIMEREPORT_EXPORTS)
    target_compile_definitions(${LIMEREPORT_TARGET} INTERFACE LIMEREPORT_IMPORTS)
endif()
add_library(LimeReport::LimeReport ALIAS ${LIMEREPORT_TARGET})

target_sources(${LIMEREPORT_TARGET} PRIVATE ${LIMEREPORT_SOURCES} ${LIMEREPORT_HEADERS} ${LIMEREPORT_RESOURCES})

set(LIMEREPORT_QML_FILES
    ${LR}/quick/qml/ReportPreview.qml
    ${LR}/quick/qml/PreviewWindow.qml
    ${LR}/quick/qml/ReportDesigner.qml
    ${LR}/quick/qml/DesignerWindow.qml
    ${LR}/quick/qml/DesignerCanvasView.qml
    ${LR}/quick/qml/PropertyInspector.qml
    ${LR}/quick/qml/PropertyEditor.qml
    ${LR}/quick/qml/ObjectTree.qml
    ${LR}/quick/qml/DataBrowser.qml
    ${LR}/quick/qml/TreeRow.qml
    ${LR}/quick/qml/ConnectionDialog.qml
    ${LR}/quick/qml/DataSourceDialog.qml
    ${LR}/quick/qml/VariableDialog.qml
    ${LR}/quick/qml/ContentEditorDialog.qml
    ${LR}/quick/qml/PopupMenuHost.qml
    ${LR}/quick/qml/IconButton.qml
)
foreach(qmlfile ${LIMEREPORT_QML_FILES})
    get_filename_component(qmlname ${qmlfile} NAME)
    set_source_files_properties(${qmlfile} PROPERTIES QT_RESOURCE_ALIAS ${qmlname})
endforeach()

qt_add_qml_module(${LIMEREPORT_TARGET}
    URI LimeReport
    VERSION 2.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES ${LIMEREPORT_QML_FILES}
    NO_PLUGIN
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/LimeReport
)

target_compile_definitions(${LIMEREPORT_TARGET} PRIVATE HAVE_QT6 HAVE_REPORT_DESIGNER USE_QJSENGINE _CRT_SECURE_NO_WARNINGS
    DEFAULT_ITEM_PADDING=${DEFAULT_ITEM_PADDING})

target_link_libraries(${LIMEREPORT_TARGET} PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Xml
    Qt6::Sql
    Qt6::Svg)

if (ENABLE_ZINT)
    target_link_libraries(${LIMEREPORT_TARGET} PRIVATE QZint)
    target_compile_definitions(${LIMEREPORT_TARGET} PRIVATE HAVE_ZINT)
endif()

target_include_directories(${LIMEREPORT_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/limereport>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/limereport>
    PRIVATE
        ${LR}
        ${LR}/base
        ${LR}/bands
        ${LR}/items
        ${LR}/serializators
        ${LR}/quick
)

install(TARGETS ${LIMEREPORT_TARGET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/limereport)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/limereport/version.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/limereport)
install(DIRECTORY ${CMAKE_BINARY_DIR}/qml/LimeReport DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/qml)

if (LIMEREPORT_BUILD_DESIGNER)
    add_subdirectory(designer)
endif()
if (LIMEREPORT_BUILD_DEMO)
    add_subdirectory(demo_r1)
endif()
if (LIMEREPORT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
