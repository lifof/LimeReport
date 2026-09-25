set(LRDESIGNER_FILES
    designersettingmanager.h
    designersettingmanager.cpp
    main.cpp
)
if (WIN32)
    list(APPEND LRDESIGNER_FILES mainicon.rc)
endif()

qt_add_executable(LRDesigner ${LRDESIGNER_FILES})

target_link_libraries(LRDesigner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    LimeReport::LimeReport
)

install(TARGETS LRDesigner RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
