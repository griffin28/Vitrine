set(UI_SRCS
    CollapsibleLogWidget.h
    CollapsibleLogWidget.cpp
    AppMainWindow.h
    AppMainWindow.cpp
    VulkanWindow.h
    VulkanWindow.cpp
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
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
)