# (C) 2022 ketteQ
#
# Author: Giancarlo Chiappe
# Configuration
configure_file (
        "${PROJECT_SOURCE_DIR}/src/imcx/src/config.h.in"
        "${PROJECT_BINARY_DIR}/config.h"
)
include_directories(${PROJECT_BINARY_DIR})
# PGXS Extension Info
set(ExtensionLib kq_imcx)
set(Version 0.1)
set(Description "KetteQ In-Memory Calendar Extension (IMCX)")
# Common Sources
set(CommonSrc src/common/util.c src/calendar/cache.c src/calendar/math.c)
# Define the meta for the PGXS plugin.
# This macro also sets the includes and links to PGXS.
add_postgresql_extension(
        ${ExtensionLib}
        VERSION ${Version}
        COMMENT ${Description}
        SOURCES ${CommonSrc} pgxs/kq_imcx.c
        SCRIPTS pgxs/kq_imcx--0.1.sql)

# Testing is disabled, a migration to a postgres-runtime test suite is rq, for example Testgres.

# Test Target Configuration
#set(TestBin imcx_test)
#set(TestSrc test/calendar_cache_tests.cpp)
# Build the Test Executable
#add_executable(${TestBin} ${CommonSrc} ${TestSrc})
#set(_link_flags "${PostgreSQL_SHARED_LINK_OPTIONS}")
#foreach(_dir ${PostgreSQL_SERVER_LIBRARY_DIRS})
#    set(_link_flags "${_link_flags} -L${_dir}")
#endforeach()
#message(STATUS "${TestBin} -> _link_flags = ${_link_flags}")
#set_target_properties(
#        ${TestBin}
#        PROPERTIES PREFIX ""
#        LINK_FLAGS "${_link_flags}"
#        POSITION_INDEPENDENT_CODE ON)
#target_include_directories(
#        ${TestBin}
#        PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
#        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
#)
#message(STATUS "${TestBin} -> PG Server Include Dirs = ${PostgreSQL_SERVER_INCLUDE_DIRS}")
#target_link_libraries(
#        ${TestBin} gtest_main PkgConfig::deps
#)
#message(STATUS "${TestBin} -> ${PostgreSQL_SERVER_INCLUDE_DIRS} | ${PostgreSQL_SERVER_LIBRARY_DIRS} | ${PostgreSQL_LIBRARIES}")

#target_include_directories(${TestBin} PUBLIC ${PostgreSQL_SERVER_INCLUDE_DIRS})
#target_link_directories(${TestBin} PUBLIC ${PostgreSQL_SERVER_LIBRARY_DIRS})
#target_link_libraries(${TestBin} gtest_main PkgConfig::deps ${PostgreSQL_LIBRARIES})
# Add Tests
#add_test(
#        NAME ${TestBin}
#        COMMAND ${TestBin}
#)