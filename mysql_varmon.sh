#!/bin/bash

# mysql_varmon.sh
#
# Monitor changing mysqld variables.
# Use an SSH tunnel when connecting to a remote host.
#
# usage:
#             ./mysql_varmon.sh
#             Ctrl+C to exit
#
# author      Martin Latter
# copyright   Martin Latter 19/08/2020
# version     0.01
# license     GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
# link        https://github.com/Tinram/MySQL.git


version="v.0.01"
date="20200819"
mysqlPort="3306" # for remote host not listening on 3306; for 'localhost' MySQL connects with sockets (override: 127.0.0.1)
scriptName=$0


showHelp()
{
	echo -e "\nmysql_varmon $version $date\nmonitor mysqld variables\n"
	echo -e "usage: $scriptName\n"
}


main()
{
	if [[ $1 == "-h" || $1 == "--help" ]]; then
		showHelp
		exit 1
	fi

	echo -e "\nmysql_varmon $version\n"

	echo -n "host: (blank = localhost) "
	read -r mysqlHost
	echo -n "username: "
	read -r mysqlUser
	echo -n "password: "
	read -sr mysqlPass


	if [[ ! $mysqlHost ]]; then
		mysqlHost="localhost"
	fi

	while :
	do

		clear

		echo -e "\n\n~~ THREADS ~~\n"
		mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
			SHOW VARIABLES WHERE variable_name = 'thread_cache_size';
			SHOW STATUS WHERE variable_name LIKE 'Threads_%'"

		echo -e "\n\n~~ CONNECTIONS ~~\n"
		mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
			SHOW VARIABLES WHERE variable_name LIKE 'max_connect%';
			SHOW STATUS WHERE variable_name = 'Max_used_connections';
			SHOW STATUS WHERE variable_name = 'Aborted_connects';
			SHOW STATUS WHERE variable_name = 'Locked_connects';
			SHOW VARIABLES WHERE variable_name = 'slow_launch_time';
			SHOW STATUS WHERE variable_name ='Slow_launch_threads'";

		echo -e "\n"

		sleep 1

	done
}


main "$@"
