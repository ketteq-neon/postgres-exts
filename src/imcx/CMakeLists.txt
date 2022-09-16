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
set(This kq_imcx)
set(Version 0.1)
set(Description "KetteQ In-Memory Calendar Extension (IMCX)")

# Common Sources
set(CommonSrc src/common/util.c src/calendar/cache.c src/calendar/math.c)

# Define the meta for the PGXS plugin. TODO: Add Regression Tests
# This macro also sets the includes and links to PGXS.
add_postgresql_extension(
        ${This}
        VERSION ${Version}
        COMMENT ${Description}
        SOURCES ${CommonSrc} pgxs/kq_imcx.c
        SCRIPTS pgxs/kq_imcx--0.1.sql)
# Link to Other C Libraries (GLib2.0)
target_link_libraries(${This} PkgConfig::deps)


# TODO: Make Test executable linked with postgresql's server library

# Test Target Configuration
set(ThisTest imcx_test)
set(TestSrc test/calendar_cache_tests.cpp pgxs/kq_imcx.h)
# Build the Test Executable
add_executable(${ThisTest} ${CommonSrc} ${TestSrc})
set(_link_flags "${PostgreSQL_SHARED_LINK_OPTIONS}")
foreach(_dir ${PostgreSQL_SERVER_LIBRARY_DIRS})
    set(_link_flags "${_link_flags} -L${_dir}")
endforeach()

message(STATUS "${ThisTest} -> _link_flags = ${_link_flags}")
set_target_properties(
        ${ThisTest}
        PROPERTIES PREFIX ""
        LINK_FLAGS "${_link_flags}"
        POSITION_INDEPENDENT_CODE ON)
target_include_directories(
        ${ThisTest}
        PRIVATE ${PostgreSQL_SERVER_INCLUDE_DIRS}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
        ${ThisTest}
        ${PostgreSQL_LIBRARIES} gtest_main PkgConfig::deps
)
# Add Tests
add_test(
        NAME ${ThisTest}
        COMMAND ${ThisTest}
)