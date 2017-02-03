
# MySQL

#### MySQL utility scripts


+ mysql\_vars.sh
+ schema\_splitter.php
+ table\_bench/table\_bench.php


---


## *mysql_vars.sh*


### Purpose

Display important mysqld variables.

Uses Bash for server portability.


### Set-up

    chmod 744 mysql_vars.sh


### Usage

    ./mysql_vars.sh

prompts for host, username, and password (not echoed) of the mysql server.


#### Usage on Windows

On Windows, via a PuTTY connection, the script output may exceed PuTTY's scrollback limit. I chose an interactive connection input for my usage on Linux-to-Linux (avoiding switches). However, this does mean stdout redirection is blocked.

Short of rewriting the command-line parsing, a workaround is:

    ./mysql_vars.sh | tee myvars.txt


## *schema_splitter.php*


### Purpose

PHP 7 CLI script to split a large MySQL schema file dump by table definitions into separate .sql files.


### Usage

    php -f schema_splitter.php <filename>

or if made executable:

    ./schema_splitter.php <filename>


## *table_bench.php*


### Purpose

+ Time row inserts into a MySQL table through configuration toggles controlling connection API, transactions etc.

+ Provides a timing harness for altering mysqld variables.

Timings are often highly variable, so require much averaging. Nevertheless, large time differences are noticeable for prepared statements and my.cnf variables adjusted for INSERTs.


### Usage

1. `cd table_bench`

2. Set the database connection parameters in *setup.php*

3. `php -f setup.php`

4. Set the parameters in *table_bench/config.php*

5. `php -f table_bench.php`

The above PHP files can also be executed through a web server.


## License

Scripts released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
