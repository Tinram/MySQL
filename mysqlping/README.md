
# mysqlping

#### Continuous MySQL pinger.


## Purpose

The command `mysqladmin ping` pings a MySQL instance, yet I wanted something more than just a '*mysqld is alive*' single response message. Especially if a network connection to the database is flaky.

*mysqlping* by default pings a MySQL connection once a second. With the `-f` switch, the program generates a flood of pings.


## OS Support

+ Linux x64


## Usage

```bash
    ./mysqlping -u <username> [-h <host>] [-f] [-p <port>]

    ./mysqlping -u root

    ./mysqlping --help
```

If the host switch is omitted, *mysqlping* attempts to connect to a localhost instance of *mysqld*.

<kbd>Ctrl</kbd> + <kbd>C</kbd> to exit.


## License

*mysqlping* is released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).
