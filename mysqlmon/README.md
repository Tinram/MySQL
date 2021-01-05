
# mysqlmon

#### *mysqld* counters monitor.


[1]: https://tinram.github.io/images/mysqlmon.png
![mysqlmon][1]

<br>

## Purpose

Created to continously monitor *mysqld* counters during Sysbench load testing.


## OS Support

+ Linux x64


## Usage

```bash
    ./mysqlmon -u <username> [-h <host>] [-p <port>]

    ./mysqlmon -u root

    ./mysqlmon --help
```

If the host switch `-h` is omitted, *mysqlmon* attempts to connect to a localhost instance of *mysqld*.

<kbd>Ctrl</kbd> + <kbd>C</kbd> to exit.


## License

*mysqlmon* is released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
