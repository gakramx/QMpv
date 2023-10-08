cmake_minimum_required(VERSION 3.16)

project(QMpv VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Quick REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)

add_library(QMpv
    qmpv.cpp
    qmpv.h
    qthelper.hpp
)

target_include_directories(QMpv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(QMpv
    PRIVATE ${MPV_LIBRARIES}
    Qt6::Quick
)
