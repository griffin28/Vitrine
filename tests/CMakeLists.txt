include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_SRCS
    TestAnariRenderer.cpp)

add_executable(unit_tests ${TEST_SRCS})

target_include_directories(unit_tests
    PRIVATE
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Gui_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/../src/renderer
        ${CMAKE_CURRENT_LIST_DIR}/../src/core
        ${CMAKE_CURRENT_LIST_DIR}/../src/camera
        ${CMAKE_CURRENT_LIST_DIR}/../src/ui)

target_link_libraries(unit_tests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
        anari::anari
        renderer
        loader
        core
        camera
        gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
