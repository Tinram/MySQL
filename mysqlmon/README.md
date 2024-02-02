
# MySQLMon

#### MySQL instance counters monitor.

<br>

[1]: https://tinram.github.io/images/mysqlmon.png
![mysqlmon][1]

<br>

## Purpose

Originally created to continuously monitor MySQL counters during *Sysbench* load testing.

Now it serves as a quick overview of activity and contention on Aurora instances.


## OS Support

+ Linux x64
+ MacOS
	+ `make` works on recent Mac / *Homebrew* versions
    + else if missing libraries:
        + `brew --prefix <lib>` # = path
        + `-L/<path>` in *make*-generated *Clang* call


## Usage

```bash
    ./mysqlmon -u <username> [-h <host>] [-p <port>]

    ./mysqlmon -u root

    ./mysqlmon --help
```

If the host switch `-h` is omitted, *mysqlmon* attempts to connect to a localhost instance of *mysqld*.

For Aurora connections, make sure the host is a read-write endpoint to provide full stats (not a read-only).


<kbd>Ctrl</kbd> + <kbd>C</kbd> to exit.


## License

*MySQLMon* is released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
