# Postgres Extensions (PGXS)

- Written in C.
- Includes PGXS CMake build environment.
- New extensions can be added easily.

*(C) 2023 KetteQ*

# Build Requirements

This extension should be compiled and run in a Linux host. Building and running in macOS or Windows hosts is not
supported but can be implemented if required.

## Ubuntu/Debian Linux

1. Install PostgreSQL Repository:
   ```bash
   sudo apt install curl ca-certificates gnupg
   curl https://www.postgresql.org/media/keys/ACCC4CF8.asc | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/apt.postgresql.org.gpg >/dev/null
   sudo sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list'
   sudo apt update
   ```
2. Install development dependencies:
   ```bash
    sudo apt install build-essential pkg-config libpq-dev libpam-dev libxslt-dev liblz4-dev libreadline-dev libkrb5-dev cmake
    ```
3. Pick one of the target versions for your extension. Extensions are linked to the server version, they are not
   interchangeable as the features between them are not compatible and are enabled statically at build time.
    - Postgres 14
      ```bash
      sudo apt install postgresql-server-dev-14 postgresql-client-14
      ```
    - Postgres 15:
      ```bash
      sudo apt install postgresql-server-dev-15 postgresql-client-15
      ```

   **IMPORTANT** - CMake will always pick the latest Postgres libraries when building.

4. Clone this repo.
5. `cd` to newly cloned repo.
6. If not using an IDE (such as CLion from Jetbrains) create `build` folders:

    ```bash
    mkdir build 
    ```
7. CD to the newly created build folder and generate Makefiles using `cmake`:

    ```bash
    cd build
    cmake .. 
    ```

   You can use `cmake -DCMAKE_BUILD_TYPE=Debug` to include debug symbols in the output binary. This will disable all
   optimizations of the C compiler.
   <br/><br/>

8. Run `make` to create the extension shared object.

    ```bash
    make -j8
    ```

* Other build helpers such as `ninja` can be used as well.

# Extensions And Features

| Extension Name                      | `CREATE` Extension Name | Features                                                                                                                                 |
|-------------------------------------|-------------------------|------------------------------------------------------------------------------------------------------------------------------------------|
| In-Memory Calendar Extension (IMCX) | `kq_imcx`               | - Stores cache dynamically using PostgreSQL Shared Memory.<br/> - Provides very fast calendar calculation functions.<br/> - Thread safe. |

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

To use the extension with a non-superuser account, you should do a one-time operation, connect first using the
superuser account, switch to the target client database and from there issue the create extension command.
After that, the extension will be available to any user that has access to that DB.
That operation will survive between server restarts and library updates.

Is not recommended to give superuser powers to an account just to enable the extension.

After the extension is enabled the following functions will be available from the SQL-query interface:

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
SELECT * FROM kq_calendar_cache_info();
                   property                    |                                                                                                value                                                                                                 
-----------------------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 PostgreSQL SDK Version                        | 150001
 Version                                       | 0.1.2
 Release Build                                 | Yes
 Cache Available                               | Yes
 Slice Cache Size (Calendar ID Count)          | 13
 Entry Cache Size                              | 16510
 Shared Memory Requested (MBytes)              | 1024
 [Q1] Get Calendar IDs                         | select min(c.id), max(c.id) from plan.calendar c
 [Q2] Get Calendar Entry Count per Calendar ID | select cd.calendar_id, count(*), (select LOWER(ct."name") from plan.calendar ct where ct.id = cd.calendar_id) "name" from plan.calendar_date cd group by cd.calendar_id order by cd.calendar_id asc;
 [Q3] Get Calendar Entries                     | select cd.calendar_id, cd."date" from plan.calendar_date cd order by cd.calendar_id asc, cd."date" asc;
(10 rows)
```

Show details about the cache:

```
SELECT * FROM kq_calendar_cache_report(false, false);

         property          |  value  
---------------------------+---------
 Calendars-Id Max          | 13
 Cache-Calendars Size      | 16510
 Calendar-Id               | 1
    Index                  | 0
    Name                   | week
    Entries                | 1983
    Page Map Size          | 868
    Page Size              | 16
 Calendar-Id               | 2
    Index                  | 1
    Name                   | month
    Entries                | 456
    Page Map Size          | 434
    Page Size              | 32
 Calendar-Id               | 3
    Index                  | 2
    Name                   | quarter
    Entries                | 152
    Page Map Size          | 432
    Page Size              | 32
 Calendar-Id               | 4
    Index                  | 3
    Name                   | year
    Entries                | 38
    Page Map Size          | 423
    Page Size              | 32
 Calendar-Id               | 13
    Index                  | 12
    Name                   | day
    Entries                | 13881
    Page Map Size          | 869
    Page Size              | 16
 Missing Calendars (id==0) | 8
(33 rows)
```

Invalidating the cache will clear memory and execute again the load queries. After
the function is executed, a fresh cache is available.

```
SELECT kq_invalidate_calendar_cache();

INFO:  Cache Invalidated Successfully
 kq_invalidate_calendar_cache 
------------------------------
 
(1 row)
```

When extension is ready and slices are loaded in memory, calculation functions can
be used.

Add an interval to a date that corresponds to the quarter calendar (Slice Type), the
date must be in a PostgreSQL-supported date format.

```
SELECT kq_add_days('2008-01-15', 1, 'quarter');

 kq_add_days 
-------------
 2008-04-01
(1 row)
```

The output of this function can be used inside a normal SQL query:

```
SELECT 1 id, '2008-01-15' old_date, kq_add_days('2008-01-15', 1, 'quarter') new_date;

 id |  old_date  |  new_date  
----+------------+------------
  1 | 2008-01-15 | 2008-04-01
(1 row)
```

# External Tests

## Extensions Concurrency Test (XSCT)

**Important:** Extension must be installed in target server before running the concurrency test.

This application written in python will connect to backend database server and simulate a race condition. The
test is fully automated and is configurable via command line options.

The result of the test is provided as standard `ERROR_CODE` where `0` means all tests were run successfully. Output
summary can be collected piping `stdout`.

1. Change directory to `concurrency-test`.
   ```bash
   cd concurrency-test
   ```
2. Activate **virtualenv** (activation script may vary depending on your shell)
   ```bash
   virtualenv ./venv/
   . venv/bin/activate
   ```
3. Install required modules
   ```bash
   pip3 install -r requirements.txt
   ```
4. Run the `xsct.suite` module from command line
   ```bash
   python3 -m xsct.suite
   ```
   Read the provided options to test the extension.

This application can be run in any Python supported operating system, if running in Windows, use the correct
`virtualenv` binaries.

# Architecture

Postgres' extensions are handled by a "bridge" (or main) C file with functions that must be mapped into the
"extension mapping" SQL file that will make these C functions available from the SQL query interface.