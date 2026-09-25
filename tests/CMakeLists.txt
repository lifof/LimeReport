find_package(Qt6 REQUIRED COMPONENTS Test)

set(LR_INTERNAL_INCLUDES
    ${PROJECT_SOURCE_DIR}/limereport
    ${PROJECT_SOURCE_DIR}/limereport/base
    ${PROJECT_SOURCE_DIR}/limereport/bands
    ${PROJECT_SOURCE_DIR}/limereport/items
    ${PROJECT_SOURCE_DIR}/limereport/serializators
    ${PROJECT_SOURCE_DIR}/limereport/quick
)

function(limereport_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_include_directories(${name} PRIVATE ${LR_INTERNAL_INCLUDES})
    target_link_libraries(${name} PRIVATE Qt6::Test Qt6::Quick Qt6::Qml Qt6::Sql LimeReport::LimeReport)
    target_compile_definitions(${name} PRIVATE DEMO_DIR="${PROJECT_SOURCE_DIR}/demo_r1")
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endfunction()

limereport_add_test(tst_callbackdstest)
limereport_add_test(tst_graphicsscene)
limereport_add_test(tst_quick)
