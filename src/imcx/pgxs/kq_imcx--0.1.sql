--
-- (C) KetteQ, Inc.
--

-- Gives information about the extension
CREATE FUNCTION kq_imcx_info()
    RETURNS TABLE ("property" text, "value" text)
AS 'MODULE_PATHNAME', 'imcx_info'
    LANGUAGE C VOLATILE STRICT;

-- Clears the cache (and frees memory)
CREATE FUNCTION kq_imcx_invalidate()
    RETURNS TEXT
AS 'MODULE_PATHNAME',
    'imcx_invalidate'
LANGUAGE C IMMUTABLE STRICT;

-- Displays as Log Messages the contents of the cache.
CREATE FUNCTION kq_imcx_report(boolean, boolean)
    RETURNS TABLE ("property" text, "value" text)
AS 'MODULE_PATHNAME',
'imcx_report'
    LANGUAGE C VOLATILE STRICT;

-- Calculates next date for the given days (selects calendar by ID).
CREATE FUNCTION kq_add_days_id(date, int, int)
    RETURNS DATE
AS 'MODULE_PATHNAME',
    'imcx_add_calendar_days_by_id'
LANGUAGE C IMMUTABLE STRICT ;

-- Calculates next date for the given days (selects calendar by NAME).
CREATE FUNCTION kq_add_days(date, int, text)
    RETURNS DATE
AS 'MODULE_PATHNAME',
'imcx_add_calendar_days_by_calendar_name'
    LANGUAGE C IMMUTABLE STRICT ;