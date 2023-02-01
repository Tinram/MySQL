
# MySQL


#### Programs

+ [MySQLMon](https://github.com/Tinram/MySQL/tree/master/mysqlmon) &ndash; counters monitor
+ [MySQLTrxMon](https://github.com/Tinram/MySQL/tree/master/mysqltrxmon) &ndash; transaction monitor
+ [MySQLLockMon](https://github.com/Tinram/MySQL/tree/master/mysqllockmon) &ndash; lock monitor
+ [mysqlping](https://github.com/Tinram/MySQL/tree/master/mysqlping) &ndash; continuous pinger


#### Scripts

+ [db\_copy.sh](#dbcopy) &ndash; copy a database between servers
+ [mysql\_vars.sh](#mysqlvars) &ndash; variable display
+ [mysql\_varmon.sh](#mysqlvarmon) &ndash; variable monitor
+ [mysql\_grants.sh](#mysqlgrants)
+ [schema\_cleaner.php](#schemacleaner) &ndash; schema file clarifier
+ [schema\_summary.php](#schemasummary) &ndash; schema file info extractor
+ [schema\_splitter.php](#schemasplitter)
+ [sys\_perf\_search.py](#sysperfsearch) &ndash; search internal schemas for keywords
+ [table\_bench/table\_bench.php](#tablebench) &ndash; insert timer
+ [table\_sizer.php](#tablesizer)



---


<a id="dbcopy"></a>
## *db_copy.sh*


### Purpose

+ Copy a MySQL database between servers via *mysqlpump* (or *mysqldump*) and the PC, attempting to preserve character set and collations of the source database.
+ Avoids the human time / hassle of manual *mysqlpump* export file and reload, plus character set amnesia for migration.
+ Created to circumvent the flaky operation of *mysql-utilities*' *mysqldbcopy* (1.6.1).


### Set-up

```bash
    chmod 744 db_copy.sh
```


### Usage

```bash
    ./db_copy.sh
```

&ndash; prompts for credentials of the MySQL servers and source database.


---


<a id="mysqlvars"></a>
## *mysql_vars.sh*


### Purpose

+ Display important MySQL server variables and counters.
+ Uses Bash for server portability.
+ Output can be easily be diff'd to compare servers.


### Set-up

```bash
    chmod 744 mysql_vars.sh
```


### Usage

```bash
    ./mysql_vars.sh
```

&ndash; prompts for host, username, and password (not echoed) of the MySQL server.


#### Usage on Windows

On Windows, via a PuTTY connection, the script output may exceed PuTTY's scrollback limit. I chose an interactive connection input for my usage on Linux-to-Linux machines (avoiding switches). Nevertheless, this means that *stdout* redirection is blocked.

Short of rewriting the command-line parsing, a workaround is:

```bash
    ./mysql_vars.sh | tee myvars.txt
```


---


<a id="mysqlvarmon"></a>
## *mysql_varmon.sh*


### Purpose

Continously monitor MySQL server variables.

Uses Bash for server portability and updating ease (but possesses a flickery terminal refresh).  
This directly led to [mysqlmon](https://github.com/Tinram/MySQL/tree/master/mysqlmon).


### Set-up

```bash
    chmod 744 mysql_varmon.sh
```


### Usage

```bash
    ./mysql_varmon.sh
```

&ndash; prompts for host, username, and password (not echoed) of the MySQL server.


<kbd>Ctrl</kbd> + <kbd>C</kbd> to exit.


---


<a id="mysqlgrants"></a>
## *mysql_grants.sh*


### Purpose

List GRANTs.

Similar to *pt-show-grants*, with cleaner output.  
Connect as root user.


### Set-up

```bash
    chmod 744 mysql_grants.sh
```


### Usage

```bash
    ./mysql_grants.sh
```


---


<a id="schemacleaner"></a>
## *schema_cleaner.php*


### Purpose

PHP CLI script to reduce and clarify a schema definition file from *mysqldump* (`-d` / `--no-data`) i.e. removing *mysqldump* comments and directives. (The output file is not suitable for import into MySQL.)


### Usage

```bash
    php schema_tools/schema_cleaner.php <filename.sql>
```


---


<a id="schemasummary"></a>
## *schema_summary.php*


### Purpose

PHP CLI script to extract a brief summary of table names and foreign keys from large unwieldy schema files.

The keyword option enables searching for keywords in table names and table contents.


### Usage

```bash
    php schema_tools/schema_summary.php <filename> [keyword]
```

Example for querying a large schema file for *user* references in table names and columns:

```bash
    php schema_tools/schema_summary.php bigschema.sql user | less
```


---


<a id="schemasplitter"></a>
## *schema_splitter.php*


### Purpose

PHP CLI script to split a large MySQL schema file by table definitions into separate *.sql* files.


### Usage

```bash
    php schema_tools/schema_splitter.php <filename>
```

example:

```bash
    php schema_tools/schema_splitter.php ../schema_example/dbfilltest.sql
```


---


<a id="sysperfsearch"></a>
## *sys_perf_search.py*


### Purpose

PHP CLI script to split a large MySQL schema file by table definitions into separate *.sql* files.

Search Performance Schema, Sys schema, or Information Schema for keywords in table names and table fields.


### Usage

Edit MySQL instance connection credentials and search keyword in the script's *CONFIGURATION* section.

```bash
    python3 sys_perf_search.py
```


---


<a id="tablebench"></a>
## *table_bench.php*


### Purpose

+ Time row inserts into a MySQL table through configuration toggles controlling transactions, prepared statements etc.
+ Provides a timing harness for altering *mysqld* variables.

Timings are often highly variable, and so require much averaging. Nevertheless, large time differences are noticeable for prepared statements and *my.cnf* variables adjusted for INSERTs.


### Usage

1. `cd table_bench`

2. Set the database connection parameters in *setup.php*

3. `php setup.php`

4. Set the parameters in *table_bench/config.php*

5. `php table_bench.php`

The above PHP files can also be executed through a web server.


---


<a id="tablesizer"></a>
## *table_sizer.php*


### Purpose

PHP CLI script to query MySQL table size properties, and if timestamps are available, provide an estimation of data insertion rates.


### Usage

First, add the database connection and table parameters in the configuration section at the top of the script.

```bash
    php table_sizer.php
```

or if made executable:

```bash
    ./table_sizer.php
```


## License

Scripts and executables released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
