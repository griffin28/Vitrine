set(UI_SRCS
    CollapsibleLogWidget.h
    CollapsibleLogWidget.cpp
    AppMainWindow.h
    AppMainWindow.cpp
    AnariFrameWidget.h
    AnariFrameWidget.cpp
    AnariBackendDialog.h
    AnariBackendDialog.cpp
    CameraAxisOverlay.h
    CameraAxisOverlay.cpp
    CameraConfigDialog.h
    CameraConfigDialog.cpp
    UserPreferences.h
    PreferencesDialog.h
    PreferencesDialog.cpp
)

add_library(ui OBJECT ${UI_SRCS})

target_compile_options(ui
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)

target_include_directories(ui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../core
        ${CMAKE_CURRENT_SOURCE_DIR}/../renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/../loader
        ${CMAKE_CURRENT_SOURCE_DIR}/../camera
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        ${glm_INCLUDE_DIRS}
)

target_link_libraries(ui
    PUBLIC
        anari::anari
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets)
