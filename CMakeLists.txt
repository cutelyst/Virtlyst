cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(Virtlyst LANGUAGES CXX)

if (POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

if(WIN32)
  if(MSVC)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  endif()
endif()

find_package(Qt5 COMPONENTS Core Network Sql Xml REQUIRED)
find_package(Cutelyst2Qt5 2.7.0 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Grantlee5 REQUIRED)
pkg_check_modules(LIBVIRT REQUIRED libvirt)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB_RECURSE TEMPLATES_SRC root/*)

add_subdirectory(src)
