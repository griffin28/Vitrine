set(RENDERER_SRCS
    VulkanRenderer.cpp
)

add_library(renderer OBJECT ${RENDERER_SRCS})

target_compile_options(renderer 
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fPIC>)

target_include_directories(renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        # ${tinyobjloader_INCLUDE_DIRS}
        # ${tinygltf_INCLUDE_DIRS}
        # ${KTX_INCLUDE_DIRS}
)