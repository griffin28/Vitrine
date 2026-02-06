set(UI_SRCS
    CollapsibleLogWidget.h
    CollapsibleLogWidget.cpp
    VulkanMainWindow.h
    VulkanMainWindow.cpp
)

add_library(ui OBJECT ${UI_SRCS})
target_compile_options(ui 
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)

target_include_directories(ui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
)