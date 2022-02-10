# (C) 2022 ketteQ

set(This ketteq_extension)
set(Version 0.1)
set(Description "ketteQ PostgreSQL Extension (TBD)")

add_postgresql_extension(
        ${This}
        VERSION ${Version}
        COMMENT ${Description}
        SOURCES common/util.c calendar/cache.c calendar/math.c ext_main.c
        SCRIPTS ketteq_extension--0.1.sql)

# Link to Other C Libraries (GLib2.0)
target_link_libraries(${This} PkgConfig::deps)