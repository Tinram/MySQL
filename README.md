
# MySQL

#### MySQL utility scripts

+ mysqlvars.sh
+ schema_splitter.php


## *mysql_vars.sh*

### Purpose

Display important mysqld variables.

Uses Bash for server portability.

(Even Perl, as an alternative, would sometimes require a dependency to be installed on some servers.)

### Set-up

    chmod 744 mysql_vars.sh

### Usage

    ./mysql_vars.sh

prompts for host, username, and password (not echoed) of the mysql server.

#### Usage on Windows

On Windows, via a PuTTY connection, the script output may exceed PuTTY's scrollback limit. I chose an interactive connection input for my usage on Linux-to-Linux (avoiding switches); however, this does mean stdout redirection is blocked.

Short of rewriting the command-line parsing, a PuTTY workaround is:

    ./mysql_vars.sh | tee myvars.txt


## schema_splitter.php

### Purpose

PHP 7 CLI script to split a large MySQL schema file by table definitions into separate files.

### Usage

    php -f schema_splitter.php <filename>


## License

Scripts released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
