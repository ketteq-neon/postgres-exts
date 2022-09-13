--- InMem Calendar Extension (IMCX)
--- (C) KetteQ, Inc.
---- Giancarlo A. Chiappe Aguilar

-- Create the Regression Tests Schema and add some data
--- This is the calendar table that holds the calendar's ids and calendar's names. It will be a FK for the entries table.
CREATE TABLE calendar (id SERIAL PRIMARY KEY, name varchar NULL);

--- This are the calendars
INSERT INTO calendar VALUES(1, 'Calendar 1');
INSERT INTO calendar VALUES(2, 'Calendar 2');
INSERT INTO calendar VALUES(3, 'Calendar 3');
INSERT INTO calendar VALUES(4, 'Calendar 4');

--- Create the calendar entries table
CREATE TABLE calendar_entries (calendar_id integer NOT NULL, date date, id SERIAL PRIMARY KEY);

--- Insert the test data
---- Monthly Calendar ID# 1
INSERT INTO calendar_entries VALUES (1, '2020-02-01', 1);
INSERT INTO calendar_entries VALUES (1, '2020-03-01', 2);
INSERT INTO calendar_entries VALUES (1, '2020-01-01', 3);
INSERT INTO calendar_entries VALUES (1, '2020-04-01', 4);
INSERT INTO calendar_entries VALUES (1, '2020-05-01', 5);
INSERT INTO calendar_entries VALUES (1, '2020-06-01', 6);
INSERT INTO calendar_entries VALUES (1, '2020-07-01', 7);
INSERT INTO calendar_entries VALUES (1, '2020-08-01', 8);
INSERT INTO calendar_entries VALUES (1, '2020-09-01', 9);
INSERT INTO calendar_entries VALUES (1, '2020-10-01', 10);
INSERT INTO calendar_entries VALUES (1, '2020-11-01', 11);

---- Monthly Calendar ID #2
INSERT INTO calendar_entries VALUES (2, '2020-04-01', 12);
INSERT INTO calendar_entries VALUES (2, '2020-05-01', 13);
INSERT INTO calendar_entries VALUES (2, '2020-06-01', 14);

---- Yearly Calendar ID #3
INSERT INTO calendar_entries VALUES (3, '2001-01-01', 15);
INSERT INTO calendar_entries VALUES (3, '2002-01-01', 16);
INSERT INTO calendar_entries VALUES (3, '2003-01-01', 17);

---- Yearly Calendar ID #4
INSERT INTO calendar_entries VALUES (4, '2010-01-01', 18);
INSERT INTO calendar_entries VALUES (4, '2011-01-01', 19);
INSERT INTO calendar_entries VALUES (4, '2012-01-01', 20);
INSERT INTO calendar_entries VALUES (4, '2013-01-01', 21);
INSERT INTO calendar_entries VALUES (4, '2014-01-01', 22);
INSERT INTO calendar_entries VALUES (4, '2015-01-01', 23);
INSERT INTO calendar_entries VALUES (4, '2016-01-01', 24);
INSERT INTO calendar_entries VALUES (4, '2017-01-01', 25);
