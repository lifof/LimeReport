qt_add_executable(demo_r1 main.cpp demobackend.h demobackend.cpp r1.qrc)
if (WIN32)
    target_sources(demo_r1 PRIVATE mainicon.rc)
endif()

qt_add_qml_module(demo_r1
    URI DemoR1
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES Main.qml
)

target_compile_definitions(demo_r1 PRIVATE DEMO_REPORTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/demo_reports")
target_link_libraries(demo_r1 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    LimeReport::LimeReport
)
