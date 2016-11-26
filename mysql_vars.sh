#!/bin/bash

# mysql_vars.sh
# display important mysqld variables
# use an SSH tunnel when connecting to a remote host

# author      Martin Latter <copysense.co.uk>
# copyright   Martin Latter 24/11/2016
# version     0.12
# license     GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
# link        https://github.com/Tinram/MySQL.git


version="v.0.12"
date="20161126"
mysqlPort="3306" # for remote host not listening on 3306; for 'localhost' MySQL connects with sockets (override: 127.0.0.1)
scriptName=$0


showHelp() {

	echo -e "\nmysql_vars $version $date\ndisplay important mysqld variables\n"
	echo -e "usage: $scriptName\n"
}


main() {

	if [[ $1 == "-h" || $1 == "--help" ]]; then
		showHelp
		exit 1
	fi

	echo -e "\nmysqlvars $version\n"

	echo -n "host: (blank = localhost) "
	read -r mysqlHost
	echo -n "username: "
	read -r mysqlUser
	echo -n "password: "
	read -sr mysqlPass


	if [[ ! $mysqlHost ]]; then
		mysqlHost="localhost"
	fi


	echo -e "\n\n~~ STATUS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		STATUS";

	echo -e "\n~~ CHARSET ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES LIKE 'character_set_%';
		SHOW VARIABLES LIKE 'collation%'";

	echo -e "\n\n~~ CONNECTIONS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW STATUS LIKE '%connect%';
		SHOW VARIABLES WHERE variable_name = 'max_connections';
		SHOW VARIABLES WHERE variable_name = 'max_allowed_packet'";

	echo -e "\n\n~~ TIMEOUTS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES LIKE '%timeout%'";

	echo -e "\n\n~~ OPEN TABLES ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW STATUS LIKE 'Open%tables';
		SHOW VARIABLES WHERE variable_name = 'table_open_cache'";

	echo -e "\n\n~~ THREADS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW STATUS WHERE variable_name = 'Threads_connected';
		SHOW STATUS WHERE variable_name = 'Threads_running'";

	echo -e "\n\n~~ INNODB KEY VARS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES WHERE variable_name = 'innodb_buffer_pool_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_log_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_flush_log_at_trx_commit';
		SHOW VARIABLES WHERE variable_name = 'innodb_log_file_size'";

	echo -e "\n\n~~ INNODB STATS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW STATUS WHERE variable_name = 'innodb_buffer_pool_size';
		SHOW STATUS WHERE variable_name LIKE 'innodb_buffer_%';
		SHOW STATUS WHERE variable_name LIKE 'innodb_data_%';
		SHOW STATUS WHERE variable_name LIKE 'innodb_log_%';
		SHOW STATUS WHERE variable_name LIKE 'innodb_row%';
		SHOW VARIABLES WHERE variable_name = 'innodb_file_io_threads';
		SHOW VARIABLES WHERE variable_name = 'innodb_io_capacity'";

	echo -e "\n\n~~ INNODB MISC ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES WHERE variable_name = 'innodb_strict_mode'";

		#echo -e "\n\n~~ INNODB STATUS ~~\n"
		#mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		#SHOW ENGINE INNODB STATUS;"

	echo -e "\n\n~~ MYISAM KEY VARS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES WHERE variable_name = 'myisam_sort_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'key_buffer_size'";

	echo -e "\n\n~~ BUFFERS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES WHERE variable_name = 'sql_buffer_result';
		SHOW VARIABLES WHERE variable_name = 'join_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'read_buffer_size ';
		SHOW VARIABLES WHERE variable_name = 'bulk_insert_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'sort_buffer_size'";

	echo -e "\n\n~~ QUERY CACHE ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES WHERE variable_name = 'have_query_cache';
			# SHOW STATUS LIKE 'qcache_%';
		SHOW VARIABLES LIKE 'query_cache_%'";

	echo -e "\n"
}


main "$@"
