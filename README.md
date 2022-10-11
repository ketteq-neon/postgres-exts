# Postgres Extensions (PGXS)

- Written in C.
- Includes PGXS CMake build environment.
- New extensions can be added easily.

*(C) 2022 KetteQ*

# Build Requirements

This extension should be compiled in a Linux host. Building in macOS or Windows
hosts is not supported.

## Ubuntu/Debian Linux

1. Install development dependencies:

    ```bash
    sudo apt install build-essential pkg-config postgresql-server-dev-14 libgtk2.0-dev libpq-dev libpam-dev libxslt-dev liblz4-dev libreadline-dev libkrb5-dev cmake
    ```

2. Clone this repo.
3. `cd` to newly cloned repo.
4. If not using an IDE (such as CLion from Jetbrains) create `build` folders:

    ```bash
    mkdir build 
    ```
5. CD to the newly created build folder and generate Makefiles using `cmake`:
    
    ```bash
    cd build
    cmake .. 
    ```
   
   You can use `cmake -DCMAKE_BUILD_TYPE=Debug` to include debug symbols in the output binary. This will disable all optimizations of the C compiler.
   <br/><br/>   

6. Run `make` to create the extension shared object.

    ```bash
    make -j8
    ```
   
* Other build helpers such as `ninja` can be used as well.

# Extensions And Features

| Extension Name                       | Create Extension Name | Features                                                                                                                                 |
|--------------------------------------|-----------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| In-Memory Calendar Extension (IMCX)  | `kq_imcx`             | - Stores cache dynamically using PostgreSQL Shared Memory.<br/> - Provides very fast calendar calculation functions.<br/> - Thread safe. |

# Installation

## In-Memory Calendar Extension (IMCX)

Required files for installation:

| File                | Description              | Manual Install Path                                   |
|---------------------|--------------------------|-------------------------------------------------------|
| `kq_imcx--0.1.sql`* | Extension Mapping File   | `/usr/share/postgresql/14/extension/kq_imcx--0.1.sql` |
| `kq_imcx.control`   | Extension Control File   | `/usr/share/postgresql/14/extension/kq_imcx.control`  |
| `kq_imcx.so`        | Extension Shared Library | `/usr/lib/postgresql/14/lib/kq_imcx.so`               |

* The Extension Mapping File should be copied from the source folder.

Install as normal Postgres Extension, with release binaries built, copy
"PostgreSQL Extension Mapping File" `kq_imcx--0.1.sql`, 
"Extension Control File" `kq_imcx.control` and "Extension Shared Library" 
`kq_imcx.so` to the target PostgreSQL Extension Folder.

As an alternate method, if you want to install in the same computer that
has built the extension use the automatic installation option:

```bash
sudo make install
```

The installation script will automatically find the correct path to install the
extension.

After installation restart the database instance.

```bash
sudo systemctl restart postgresql
```

# Removal / Uninstall

Just delete all three extension files from the PostgreSQL's extensions folder. Sudo/Root access may be required.

After manual deletion of files, restart PostgreSQL Server.

# Usage

## In-Memory Calendar Extension (IMCX)

Enable the extension using the `CREATE EXTENSION IF NOT EXISTS kq_imcx` command. This command can
be emitted using `psql` using a **superuser account** (like the `postgres` account). 

To use the extension with a non-superuser account, you should connect first using the superuser account, 
switch to the target client database and from there issue the create extension command. After that, the 
extension will be available to any user that has access to that DB. 

Is not recommended to give superuser powers to an account just to enable the extension.

After the extension is enabled the following functions will be available
from the SQL-query interface:

| Function                                                                               | Description                                                               |
|----------------------------------------------------------------------------------------|---------------------------------------------------------------------------|
| kq_calendar_cache_info()                                                               | Returns information about the extension as records.                       |
| kq_invalidate_calendar_cache()                                                         | Invalidates the loaded cache.                                             |
| kq_calendar_cache_report(`showEntries bool`,`showPageMap bool `,`showSliceNames bool`) | List the cached calendars.                                                |   
| kq_add_days_by_id(`input date`, `interval int`, `slicetype-id int`)                    | Calculate the next or previous date using the calendar ID.                |
| kq_add_days(`input date`, `interval int`, `slicetype-name text`)                       | Same as the previous function but uses the calendar NAMEs instead of IDs. |

Any call to the extension functions will automatically load the slices in memory if not
already loaded or failed in the `CREATE EXTENSION` time.

### Examples

When the extension is successfully loaded and slices are in memory an information
message is shown in the console: `INFO:  KetteQ In-Memory Calendar Extension Loaded.`.

Show information about the extension status:

```
# SELECT * FROM kq_calendar_cache_info();
INFO:  KetteQ In-Memory Calendar Extension Loaded.
              property              | calendar_id 
------------------------------------+-------
 Version                            | 0.0.0
 Cache Available                    | Yes
 Slice Cache Size (SliceType Count) | 13
 Entry Cache Size (Slices)          | 16510
(4 rows)
```

Show details about the cache:

```
# SELECT * FROM kq_calendar_cache_report(false, false);
        property        |  calendar_id  
------------------------+---------
 Slices-Id Max          | 13
 Cache-Calendars Size   | 8
 SliceType-Id           | 1
    Name                | week
    Entries             | 1983
    Page Map Size       | 868
    Page Size           | 16
 SliceType-Id           | 2
    Name                | month
    Entries             | 456
    Page Map Size       | 434
    Page Size           | 32
 SliceType-Id           | 3
    Name                | quarter
    Entries             | 152
    Page Map Size       | 432
    Page Size           | 32
 SliceType-Id           | 4
    Name                | year
    Entries             | 38
    Page Map Size       | 423
    Page Size           | 32
 SliceType-Id           | 13
    Name                | day
    Entries             | 13881
    Page Map Size       | 869
    Page Size           | 16
 Missing Slices (id==0) | 8
(28 rows)
```

Invalidating the cache will clear memory and execute again the load queries. After
the function is executed, a fresh cache is available.

```
SELECT kq_invalidate_calendar_cache();
 kq_imcx_invalidate 
--------------------
 Cache Invalidated.
(1 row)
```

When extension is ready and slices are loaded in memory, calculation functions can
be used.

Add an interval to a date that corresponds to the quarter calendar (Slice Type), the
date must be in a PostgreSQL-supported date format.

```
# SELECT kq_add_days('2008-01-15', 1, 'quarter');
 kq_add_days 
-------------
 2008-04-01
(1 row)
```

The output of this function can be used inside a normal SQL query:

```
# SELECT 1 id, '2008-01-15' old_date, kq_add_days('2008-01-15', 1, 'quarter') new_date;
 id |  old_date  |  new_date  
----+------------+------------
  1 | 2008-01-15 | 2008-04-01
(1 row)
```

# Architecture

Postgres' extensions are handled by a "bridge" (or main) C file with functions that must be mapped into the 
"extension mapping" SQL file that will make these C functions available from the SQL query interface.