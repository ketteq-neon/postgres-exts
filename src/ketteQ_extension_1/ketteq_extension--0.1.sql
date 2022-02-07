-- This caches all the calendars into memory
CREATE FUNCTION kq_load_calendars()
    RETURNS TEXT
AS 'MODULE_PATHNAME',
'cache_all_calendars'
    LANGUAGE C IMMUTABLE STRICT ;

-- -- This caches only the given calendar_id into memory
-- CREATE FUNCTION kq_load_calendar(int)
--     RETURNS TEXT
-- AS 'MODULE_PATHNAME',
--     'cache_calendar_by_id'
-- LANGUAGE C IMMUTABLE STRICT ;

-- Clears the cache (and frees memory)
CREATE FUNCTION kq_clear_calendar_cache()
    RETURNS TEXT
AS 'MODULE_PATHNAME',
    'cc_reset_cache'
LANGUAGE C IMMUTABLE STRICT;

-- Displays as Log Messages the contents of the cache.
CREATE FUNCTION kq_show_calendar_cache()
    RETURNS TEXT
AS 'MODULE_PATHNAME',
'cc_get_cache'
    LANGUAGE C IMMUTABLE STRICT;



-- -- Add days to the earliest date of the given calendar.
-- -- POC Function.
-- CREATE FUNCTION kq_get_first_date_of_calendar(int)
--     RETURNS DATE
-- AS 'MODULE_PATHNAME',
--     'cc_get_first_date_of_calendar'
-- LANGUAGE C IMMUTABLE STRICT;
--
-- -- POC Function
-- CREATE FUNCTION kq_get_latest_date_of_calendar(int)
--     RETURNS DATE
-- AS 'MODULE_PATHNAME',
--     'cc_get_last_date_of_calendar'
-- LANGUAGE C IMMUTABLE STRICT ;

CREATE FUNCTION kq_add_calendar_days(date, int, int)
    RETURNS DATE
AS 'MODULE_PATHNAME',
    'kq_add_calendar_days'
LANGUAGE C IMMUTABLE STRICT ;

CREATE FUNCTION kq_add_calendar_days_by_name(date, int, text)
    RETURNS DATE
AS 'MODULE_PATHNAME',
'kq_add_calendar_days'
    LANGUAGE C IMMUTABLE STRICT ;

-- -- This caches the specified calendar_id into memory
-- CREATE FUNCTION kq_load_calendar(int)
--     RETURNS TEXT
-- AS 'MODULE_PATHNAME',
-- ''
--     LANGUAGE C IMMUTABLE STRICT ;
