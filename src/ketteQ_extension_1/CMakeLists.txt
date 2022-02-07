add_postgresql_extension(
        ketteq_extension
        VERSION 0.1
        COMMENT "ketteQ PostgreSQL Extension (TBD)"
        SOURCES common/util.c calendar/cache_util.c calendar/cache.c calendar/math.c ext_main.c
        SCRIPTS ketteq_extension--0.1.sql)

# Link to Other C Libraries
target_link_libraries(ketteq_extension PkgConfig::deps)