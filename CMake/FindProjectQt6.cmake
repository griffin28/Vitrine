# FindProjectQt6.cmake

if(DEFINED $ENV{Qt6_DIR})
    file(TO_CMAKE_PATH "$ENV{Qt6_DIR}" _Qt6_DIR)
    set(Qt6_DIR ${_Qt6_DIR} CACHE PATH "Install location of Qt6" FORCE)
    mark_as_advanced(Qt6_DIR)
endif()

set(Qt_MODULES
    Core
    Gui
    Widgets
    Test
)

find_package(Qt6 CONFIG REQUIRED COMPONENTS ${Qt_MODULES})