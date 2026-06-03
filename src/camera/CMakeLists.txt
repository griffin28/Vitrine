set(CAMERA_SRCS
    CameraConfig.h
    CameraConfig.cpp
    Camera.h
    Camera.cpp
    PerspectiveCamera.h
    PerspectiveCamera.cpp
    OrthographicCamera.h
    OrthographicCamera.cpp
)

add_library(camera OBJECT ${CAMERA_SRCS})

target_compile_options(camera
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)

target_include_directories(camera
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${glm_INCLUDE_DIRS}
)

target_link_libraries(camera
    PUBLIC
        anari::anari
        Qt6::Gui
        glm::glm)
