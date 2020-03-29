#!/bin/bash

# mysql_vars.sh
#
# Display important mysqld variables.
# Use an SSH tunnel when connecting to a remote host.

# author      Martin Latter
# copyright   Martin Latter 24/11/2016
# version     0.15
# license     GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
# link        https://github.com/Tinram/MySQL.git


version="v.0.15"
date="20200329"
mysqlPort="3306" # for remote host not listening on 3306; for 'localhost' MySQL connects with sockets (override: 127.0.0.1)
scriptName=$0


showHelp()
{
	echo -e "\nmysql_vars $version $date\ndisplay important mysqld variables\n"
	echo -e "usage: $scriptName\n"
}


main()
{
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

	echo -e "\n\n~~ CONNECTIONS AND THREADS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW STATUS LIKE '%connect%';
		SHOW VARIABLES WHERE variable_name = 'max_allowed_packet';
		SHOW VARIABLES WHERE variable_name = 'thread_cache_size';
		SHOW STATUS WHERE variable_name = 'threads_connected';
		SHOW STATUS WHERE variable_name = 'threads_running';
		SHOW VARIABLES WHERE variable_name = 'max_connections';
		SHOW VARIABLES WHERE variable_name = 'wait_timeout'";

	echo -e "\n\n~~ TIMEOUTS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES LIKE '%timeout%'";

	echo -e "\n\n~~ OPEN TABLES ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW STATUS LIKE 'Open%tables';
		SHOW VARIABLES WHERE variable_name = 'table_open_cache'";

	echo -e "\n\n~~ MYISAM KEY VARS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES WHERE variable_name = 'myisam_sort_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'key_buffer_size'";
		#SELECT
			#(SELECT variable_value FROM information_schema.global_status WHERE variable_name = 'key_reads') AS key_reads,
			#(SELECT variable_value FROM information_schema.global_status WHERE variable_name = 'key_read_requests') AS kr_requests,
			#(SELECT (1 - key_reads / kr_requests) * 100) AS key_cache_hit_percentage";

	echo -e "\n\n~~ INNODB KEY VARS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "

		SHOW VARIABLES WHERE variable_name = 'innodb_strict_mode';
		SHOW VARIABLES WHERE variable_name = 'innodb_file_format';
		SHOW VARIABLES WHERE variable_name = 'innodb_file_per_table';
		SHOW VARIABLES WHERE variable_name = 'innodb_buffer_pool_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_buffer_pool_instances';
		SHOW VARIABLES WHERE variable_name = 'innodb_change_buffer_max_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_commit_concurrency';
		SHOW VARIABLES WHERE variable_name = 'innodb_io_capacity';
		SHOW VARIABLES WHERE variable_name = 'innodb_thread_concurrency';
		SHOW VARIABLES WHERE variable_name = 'innodb_old_blocks_time';
		SHOW VARIABLES WHERE variable_name = 'innodb_log_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_log_file_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_flush_log_at_trx_commit';
		SHOW VARIABLES WHERE variable_name = 'innodb_flush_method'";

	echo -e "\n\n~~ INNODB STATS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW STATUS WHERE variable_name LIKE 'innodb_buffer_%';
		SHOW STATUS WHERE variable_name LIKE 'innodb_data_%';
		SHOW STATUS WHERE variable_name LIKE 'innodb_log_%';
		SHOW STATUS WHERE variable_name LIKE 'innodb_row%'";

	#echo -e "\n\n~~ INNODB STATUS ~~\n"
	#mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
	#SHOW ENGINE INNODB STATUS;";

	echo -e "\n\n~~ KEYS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW STATUS LIKE 'key%'";

	echo -e "\n\n~~ BUFFERS ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES WHERE variable_name = 'sql_buffer_result';
		SHOW VARIABLES WHERE variable_name = 'read_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'join_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'sort_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'bulk_insert_buffer_size'";

	echo -e "\n\n~~ QUERY CACHE ~~\n"
	mysql -u$mysqlUser -p$mysqlPass -h$mysqlHost -P$mysqlPort -e "
		SHOW VARIABLES WHERE variable_name = 'have_query_cache';
		SHOW STATUS LIKE 'qcache_%';
		SHOW VARIABLES LIKE 'query_cache_%'";

	echo -e "\n"
}


main "$@"
