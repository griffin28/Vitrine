set(CORE_SRCS
    AppUtils.h
    LogLevel.h
    AnariUtils.h
    AnariUtils.cpp
)

add_library(core OBJECT ${CORE_SRCS})

target_compile_options(core
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)

target_include_directories(core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
)

target_link_libraries(core
    PUBLIC
        anari::anari
        Qt6::Core
        Qt6::Gui)
