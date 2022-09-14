# In-Mem Calendar Extension (IMCX)

- Written in C.
- Uses GHashTable to store calendar cache in memory.
- Uses the PostgreSQL Server's memory.
- Includes PGXS CMake build environment.

*(C) 2022 KetteQ*

## Build Requirements

This extension should be compiled in a Linux host. Building in macOS or Windows
hosts is not supported.

### Ubuntu/Debian Linux

1. Install development dependencies:

    ```bash
    sudo apt install libpam-dev libxslt-dev liblz4-dev libreadline-dev libkrb5-dev
    ```

2. Clone repo
3. CD to repo root dir
4. Clone Google Test

    ```bash
    git clone https://github.com/google/googletest 
    ```
5. If not using an IDE (such as CLion from Jetbrains) create `build` folders:

    ```bash
    mkdir build 
    ```
6. CD to the newly created build folder and generate Makefiles using `cmake`
    
    ```bash
    cd build
    cmake .. 
    ```
7. Run `make` to create the extension shared object.

    ```bash
    make 
    ```
   
* Other build helpers such as `ninja` can be used as well.

# Installation

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

# Use

Enable the extension using the `CREATE EXTENSION IF NOT EXISTS kq_imcx` command. This command can
be emitted using `psql` using a **superuser account** (like the `postgres` account). 

To use the extension with a non-superuser account, you should connect first using the superuser account, 
switch to the target client database and from there issue the create extension command. After that, the 
extension will be available to any user that has access to that DB. 

Is not recommended to give superuser powers to an account just to enable the extension.

After the extension is enable the following functions will be available
from the SQL-query interface:

| Function                                            | Description                                                               |
|-----------------------------------------------------|---------------------------------------------------------------------------|
| kq_imcx_info()                                      | Returns information about the extension as records.                       |
| kq_load_calendars()                                 | Reads the calendar table and loads it into memory.                        |
| kq_clear_calendar_cache()                           | Invalidates the loaded cache.                                             |
| kq_show_calendar_cache()                            | List the cached calendars.                                                |   
| kq_add_calendar_days(`date`, `int`, `int`)          | Calculate the next or previous date using the calendar ID.                |
| kq_add_calendar_days_by_name(`date`, `int`, `text`) | Same as the previous function but uses the calendar NAMEs instead of IDs. |


# Architecture

Postgres extensions are driven by a "bridge" (or main) C file with functions which
will be then mapped in the "extension mapping" SQL file that will make mapped "bridge" C functions 
available from the SQL-query interface.

| File                           | Description                       |
|--------------------------------|-----------------------------------|
| src/imcx/pgxs/ext_main.c       | Extension C Bridge (Main)         |
| src/imcx/pgxs/kq_imcx--0.1.sql | PostgreSQL Extension Mapping File |
| src/imcx/src/                  | Extension C Source Files          |

