# (C) 2022 ketteQ
#
# Author: Giancarlo Chiappe

set(This ketteq_extension)
set(Version 0.1)
set(Description "ketteQ PostgreSQL Extension (TBD)")

# Define the meta for the PGXS plugin. TODO: Add Regression Tests
add_postgresql_extension(
        ${This}
        VERSION ${Version}
        COMMENT ${Description}
        SOURCES src/common/util.c src/calendar/cache.c src/calendar/math.c src/ext_main.c
        SCRIPTS src/ketteq_extension--0.1.sql)

# Link to Other C Libraries (GLib2.0)
target_link_libraries(${This} PkgConfig::deps)

# Add Tests
