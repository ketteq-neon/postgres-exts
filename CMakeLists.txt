cmake_minimum_required(VERSION 3.6)
project(ketteQ_PostgreSQL_Extensions)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(PostgreSQL REQUIRED)

# GLIB 2.0
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PostgreSQL_INCLUDE_DIRS}
                    ${PostgreSQL_SERVER_INCLUDE_DIRS})

enable_testing()
add_subdirectory(src)