
# mysqlmon

#### mysqld parameters monitor.


## Purpose

Created to display key *mysqld* parameters while Sysbench load testing.


## OS Support

+ Linux x64


## Usage

```bash
    ./mysqlmon -u <username> [-h <host>] [-p <port>]

    ./mysqlmon --help
```

If the host switch is omitted, *mysqlmon* attempts to connect to a localhost instance of *mysqld*.


## License

*mysqlping* is released under the [GPL v.3](https://www.gnu.org/licenses/gpl-3.0.html).