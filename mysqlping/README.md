
# mysqlping

#### MySQL pinger.


## Purpose

`mysqladmin ping` pings MySQL, yet I wanted something more than just a single '*mysqld is alive*' response.

*mysqlping* pings a MySQL connection once a second, or using the `-f` switch, generates a flood of pings.


## OS Support

+ Linux x64


## Usage

```bash
    ./mysqlping -u <username> [-h <host>] [-f] [-p <port>]

    ./mysqlping --help
```

If the host switch is omitted, *mysqlping* attempts to connect to a localhost instance of *mysqld*.

<kbd>Ctrl</kbd> + <kbd>C</kbd> to exit.


## License

*mysqlping* is released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
