
# MySQL


#### MySQL utility programs.

+ [mysqlmon](https://github.com/Tinram/MySQL/tree/master/mysqlmon)
+ [mysqlping](https://github.com/Tinram/MySQL/tree/master/mysqlping)


#### MySQL utility scripts.

+ [mysql\_vars.sh](#mysqlvars)
+ [mysql\_varmon.sh](#mysqlvarmon)
+ [mysql\_grants.sh](#mysqlgrants)
+ [schema\_summary.php](#schemasummary)
+ [schema\_splitter.php](#schemasplitter)
+ [table\_bench/table\_bench.php](#tablebench)
+ [table\_sizer.php](#tablesizer)
+ [innodb\_status\_parser.py](#innodbstatusparser)


---


<a id="mysqlvars"></a>
## *mysql_vars.sh*


### Purpose

Display important mysqld variables.

Uses Bash for server portability.

Use Meld / differ to compare servers.


### Set-up

```bash
    chmod 744 mysql_vars.sh
```


### Usage

```bash
    ./mysql_vars.sh
```

&ndash; prompts for host, username, and password (not echoed) of the mysql server.


#### Usage on Windows

On Windows, via a PuTTY connection, the script output may exceed PuTTY's scrollback limit. I chose an interactive connection input for my usage on Linux-to-Linux (avoiding switches). However, this does mean stdout redirection is blocked.

Short of rewriting the command-line parsing, a workaround is:

```bash
    ./mysql_vars.sh | tee myvars.txt
```


---


<a id="mysqlvarmon"></a>
## *mysql_varmon.sh*


### Purpose

Continously monitor mysqld variables.

Uses Bash for server portability.


### Set-up

```bash
    chmod 744 mysql_varmon.sh
```


### Usage

```bash
    ./mysql_varmon.sh
```

&ndash; prompts for host, username, and password (not echoed) of the mysql server.


<kbd>Ctrl</kbd> + <kbd>C</kbd> to exit.


---


<a id="mysqlgrants"></a>
## *mysql_grants.sh*


### Purpose

List GRANTs.

Similar to, yet simpler than, *pt-show-grants*.  
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


<a id="schemasummary"></a>
## *schema_summary.php*


### Purpose

PHP CLI script to extract a brief summary of table names and foreign keys from large schema files.

The optional keyword enables the script to search a large schema for keywords in table names (and table contents).


### Usage

```bash
    php schema_summary.php <filename> [keyword]
```

example for querying a large schema file for *user* references in table names and table fields/keys:

```bash
    php schema_summary.php bigschema.sql user | less
```


---


<a id="schemasplitter"></a>
## *schema_splitter.php*


### Purpose

PHP CLI script to split a large MySQL schema file by table definitions into separate .sql files.


### Usage

```bash
    php schema_splitter.php <filename>
```

example:

```bash
    php schema_splitter.php schema_example/dbfilltest.sql
```


---


<a id="tablebench"></a>
## *table_bench.php*


### Purpose

+ Time row inserts into a MySQL table through configuration toggles controlling connection API, transactions etc.

+ Provides a timing harness for altering mysqld variables.

Timings are often highly variable, so require much averaging. Nevertheless, large time differences are noticeable for prepared statements and my.cnf variables adjusted for INSERTs.


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


---


<a id="innodbstatusparser"></a>
## *innodb_status_parser.py*


### Purpose

Extract and highlight some of the important items of the InnoDB monitor output.


### Requirements

Python 2 or Python 3.


### Set-up

Check CONFIG section parameters, and edit if required.


### Usage

```bash
    python innodb_status_parser.py
```

or


```bash
    chmod 744 innodb_status_parser.py
    ./innodb_status_parser.py
```

prompts for the password (not echoed) of the MySQL server.

If `USE_FILE` in the top configuration section is set to `True`, the script will attempt to parse a previous file dump (`FILENAME` default is `example.txt`) of the InnoDB monitor output.


---


## License

Scripts and programs released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
