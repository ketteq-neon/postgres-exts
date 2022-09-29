--
-- (C) KetteQ, Inc.
--

-- Gives information about the extension
CREATE FUNCTION kq_calendar_cache_info()
    RETURNS TABLE ("property" text, "value" text)
AS 'MODULE_PATHNAME', 'imcx_info'
    LANGUAGE C STABLE STRICT;

-- Clears the cache (and frees memory)
CREATE FUNCTION kq_invalidate_calendar_cache()
    RETURNS TEXT
AS 'MODULE_PATHNAME',
    'imcx_invalidate'
LANGUAGE C STABLE STRICT;

-- Displays as Log Messages the contents of the cache.
CREATE FUNCTION kq_calendar_cache_report(boolean, boolean)
    RETURNS TABLE ("property" text, "value" text)
AS 'MODULE_PATHNAME',
'imcx_report'
    LANGUAGE C STABLE STRICT;

-- Calculates next date for the given days (selects calendar by ID).
CREATE FUNCTION kq_add_days_by_id(date, int, int)
    RETURNS DATE
AS 'MODULE_PATHNAME',
    'imcx_add_calendar_days_by_id'
LANGUAGE C STABLE STRICT ;

-- Calculates next date for the given days (selects calendar by NAME).
CREATE FUNCTION kq_add_days(date, int, text)
    RETURNS DATE
AS 'MODULE_PATHNAME',
'imcx_add_calendar_days_by_calendar_name'
    LANGUAGE C STABLE STRICT ;