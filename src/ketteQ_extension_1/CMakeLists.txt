# (C) 2022 ketteQ
#
# Author: Giancarlo Chiappe

# PGXS Extension Info
set(This ketteq_extension)
set(Version 0.1)
set(Description "ketteQ PostgreSQL Extension (TBD)")

# Common Sources
set(CommonSrc src/common/util.c src/calendar/cache.c src/calendar/math.c)

# Define the meta for the PGXS plugin. TODO: Add Regression Tests
add_postgresql_extension(
        ${This}
        VERSION ${Version}
        COMMENT ${Description}
        SOURCES ${CommonSrc} pgxs/ext_main.c
        SCRIPTS pgxs/ketteq_extension--0.1.sql)
# Link to Other C Libraries (GLib2.0)
target_link_libraries(${This} PkgConfig::deps)

# Test Target Configuration
set(ThisTest ketteq_extension_test)
set(TestSrc test/calendar_cache_tests.cpp)
# Build the Test Executable
add_executable(${ThisTest} ${CommonSrc} ${TestSrc})
target_link_libraries(
        ${ThisTest}
        PUBLIC
        gtest_main
        PkgConfig::deps
)
# Add Tests
add_test(
        NAME ${ThisTest}
        COMMAND ${ThisTest}
)