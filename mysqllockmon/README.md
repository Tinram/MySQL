
# MySQLLockMon

#### MySQL Lock Monitor.

<br>


## Purpose

View MySQL locks.

Created to work through the case studies in *MySQL Concurrency* by Jesper Wisborg Krogh, Apress 2021.

Instead of multiple SQL queries and text output to evaluate locks, as used in Jesper's book, *MySQLLockMon* is a TUI that displays and updates the locks in real-time.

<img src="https://tinram.github.io/images/mlm_mysql_concurrency.jpg" alt="MySQL Concurrency, Apress">

<br>


## Requirements

+ Linux machine.
+ User privileges granted to access the *performance schema* of the MySQL server.

<br>

### Transactions

<img src="https://tinram.github.io/images/mlm_transactions.gif" alt="transactions">

<br>

### InnoDB Lock Waits

<img src="https://tinram.github.io/images/mlm_innodb_locks.gif" alt="innodb lock waits">

<br>

### Metadata Locks

<img src="https://tinram.github.io/images/mlm_meta_locks.gif" alt="metadata locks">

<br>

### Table Lock Waits

<img src="https://tinram.github.io/images/mlm_table_locks.gif" alt="table lock waits">

<br>


## Usage

```bash
    ./mysqllockmon -u <username> [-h <host>] [-t <time>] [-p <port>]

    ./mysqllockmon -u root

    ./mysqllockmon --help
```


If the host switch `-h` is omitted, *mysqllockmon* attempts to connect to a localhost MySQL instance.

<br>

Keys: cursor keys <kbd>↑</kbd> <kbd>↓</kbd> <kbd>←</kbd> <kbd>→</kbd>  to change views.

<kbd>↑</kbd>&nbsp;&nbsp;&nbsp;*transactions*

<kbd>↓</kbd>&nbsp;&nbsp;&nbsp;*InnoDB lock waits*

<kbd>←</kbd>&nbsp;&nbsp;&nbsp;*table lock waits*

<kbd>→</kbd>&nbsp;&nbsp;&nbsp;*metadata locks*

<br>

<kbd>Ctrl</kbd> + <kbd>C</kbd> to exit.

<br>


## Limitations

*MySQLLockMon* is intended to investigate locks on a small number of concurrent queries.  
Do not use it on anything other than a development server.


## Build

### Linux

```bash
    make deps  # (if mysqlclient and ncurses libraries not already installed)

    make
```

## License

*MySQLLockMon* is released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
