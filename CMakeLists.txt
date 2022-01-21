cmake_minimum_required(VERSION 3.21)
project(WinToast)

set(CMAKE_CXX_STANDARD 14)

add_library(WinToast STATIC
        src/wintoastlib.cpp)
target_include_directories(WinToast PRIVATE
        src)
target_include_directories(WinToast PUBLIC
        include)
set_target_properties(WinToast PROPERTIES PUBLIC_HEADER
        "include/wintoastlib.h")

add_executable(WinToastConsoleExample
        example/console-example/main.cpp)
target_link_libraries(WinToastConsoleExample WinToast)

install(TARGETS WinToast WinToastConsoleExample
        PUBLIC_HEADER DESTINATION include)
