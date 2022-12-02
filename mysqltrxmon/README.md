
# MySQLTrxMon

#### MySQL Transaction Monitor.


<br>

[1]: https://tinram.github.io/images/mysqltrxmon.gif
![mysqltrxmon][1]

<br>


## Purpose

Monitor and record long-running transactions on a MySQL server.

Substitute for *Innotop* transaction output (when privileges are unavailable, or flaky Mac operation via *Homebrew*).


## Requirements

+ Linux or Mac machine.
+ User privileges granted to access the *performance schema* of the server.
+ For Aurora instances, the host must be a read-write endpoint to provide full stats (not a read-only endpoint).


## Stats

| acronym | transaction attribute                         |
| ------- | --------------------------------------------- |
| trx     | number of transactions executing              |
| hll     | history list length (size of active undo log) |
| thd     | thread ID                                     |
| ps      | process ID                                    |
| exm     | number of table rows examined                 |
| lock    | number of rows locked                         |
| mod     | number of rows modified                       |
| afft    | number of rows affected                       |
| tmpd    | temporary disk tables generated               |
| tlk     | number of tables locked                       |
| idx     | index used or not in transaction query        |
| wait    | statement event elapsed time (secs)           |
| start   | transaction start time                        |
| sec     | time since transaction started (secs)         |
| user    | user running transaction                      |


## Usage

```bash
    ./mysqltrxmon -u <username> [-h <host>] [-f <logfile>] [-t <time>] [-p <port>]

    ./mysqltrxmon -u root

    ./mysqltrxmon --help
```

If the host switch `-h` is omitted, *mysqltrxmon* attempts to connect to a localhost MySQL instance.


Visual monitor:

```bash
    ./mysqltrxmon -u johndoe -h myserver
```

Visual monitor plus log results as a CSV file in the default refresh period (250 milliseconds):

```bash
    ./mysqltrxmon -u johndoe -h myserver -f trx.log
```

Visual monitor with the refresh period reduced to 100 milliseconds:

```bash
    ./mysqltrxmon -u johndoe -h myserver -t 100
```

<br>

Keys:

<kbd>↑</kbd>&nbsp;&nbsp;&nbsp;scroll up
<br>
<kbd>↓</kbd>&nbsp;&nbsp;&nbsp;scroll down

<kbd>Ctrl</kbd> + <kbd>C</kbd>&nbsp;&nbsp;&nbsp;exit

<br>


## CSV Output

The CSV emitted is a PSV (pipe-separated value) file.  
(Pipe separators are enforced by the logged SQL statements containing commas.)

Transactions are recorded in real-time, so concurrent transactions will overlap in the PSV.

There are example Python scripts in the *utils/* directory to aggregate concurrent transactions and plot values from the PSV.


## Other

Transaction visibility and capture on busy servers is dictated by the refresh rate (`-t`). Not all fast-executing transactions will be captured.


## Build

### Linux

```bash
    make deps  # (if mysqlclient and ncurses libraries not already installed)

    make
```

### MacOS

Mac with *Homebrew* 3.6.1+:

```bash
    cd src/
    rm makefile
    mv makefile_mac makefile
    make
```

Older *Homebrew* versions can be obstreperous, with additional libraries required such as *zstd*. Compilation error output can be used to fix  (install) *Homebrew* dependencies. Included in *utils/* is an example shell script, the result of the fix process on an M1 Mac.


## License

*MySQLTrxMon* is released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
