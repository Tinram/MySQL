
# mysqltrxmon

#### MySQL Transaction Monitor

##### Experimental program for when *Innotop* is unavailable (privileges unavailable, or flaky operation via *Homebrew*).


## Purpose

Monitor and log transactions on a MySQL server.


## Requirements

+ Linux or Mac machine.
+ User privileges granted to access the *performance_schema* of the server.
+ For Aurora instances, make sure the host is a read-write endpoint to provide full stats (not a read-only).


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


<kbd>Ctrl</kbd> + <kbd>C</kbd> to exit.


## CSV Output

The CSV emitted is a PSV (pipe-separated value) file.  
Pipe separators are enforced by the logged SQL statements containing commas.

An example Python script is in *utils/* to extract and plot values from the PSV.


## Issues

*Ncurses* formatting on busy versus quiet servers.


## Build

### Linux

```bash
    make deps  # (if *mysqlclient* and *ncurses* libraries not already installed)
    make
```

### MacOS

Mac is a little trickier, with additional libraries required such as *zstd*.  The Linux *makefile* can be run, substituting *gcc* with *clang*, and then the required *Homebrew* required dependencies fixed (installed) from the error output. Included in *utils/* is an example shell script, the result of this reversal process on an M1 Mac.


## License

*mysqtrxmon* is released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).