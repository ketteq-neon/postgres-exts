# (C) 2022 ketteQ
#
# Author: Giancarlo Chiappe
#
#
cmake_minimum_required(VERSION 3.16)
project(KetteQ_Postgres_Extensions
        VERSION "0.1.1"
        LANGUAGES C CXX)

# Add PGXS CMake Module (in ./cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# Set compiler flags
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find PG Headers
find_package(PostgreSQL REQUIRED)

# Add PGXS Plugin Includes
include_directories(${PostgreSQL_SERVER_INCLUDE_DIRS})
link_directories(${PostgreSQL_SERVER_LIBRARY_DIRS})

# Testing is disabled, a migration to a postgres-runtime test suite is rq, for example Testgres.

# Enable Testing (CTest)
# enable_testing()
# Add GoogleTest Suite
# add_subdirectory(googletest)
# Add Sources
add_subdirectory(src)