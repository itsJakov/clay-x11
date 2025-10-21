cmake_minimum_required(VERSION 3.27)
project(x11_demo C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED xcb)

add_executable(x11_demo main.c)
target_include_directories(x11_demo PRIVATE ${XCB_INCLUDE_DIR})
target_link_libraries(x11_demo PRIVATE ${XCB_LIBRARIES} xcb-icccm xcb-util cairo)