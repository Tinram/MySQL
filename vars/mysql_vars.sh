#!/bin/bash

# mysql_vars.sh
#
# Display important mysqld variables.
# Use an SSH tunnel when connecting to a remote host.
# ./mysql_vars.sh | tee myvars.txt
#
# author      Martin Latter
# copyright   Martin Latter 24/11/2016
# version     0.26
# license     GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
# link        https://github.com/Tinram/MySQL.git


name="mysql_vars"
version="v.0.26"
date="20221207"
mysqlPort="3306" # for remote host not listening on 3306; for 'localhost' MySQL connects with sockets (override: 127.0.0.1)
scriptName=$0
declare -i v8=0


showHelp()
{
	echo -e "\n$name $version $date\ndisplay important mysqld variables\n"
	echo -e "usage: $scriptName\n"
}


main()
{
	if [[ $1 == "-h" || $1 == "--help" ]]; then
		showHelp
		exit 1
	fi

	echo -e "\n$name $version\n"

	echo -n "host: (blank = localhost) "
	read -r mysqlHost
	echo -n "username: "
	read -r mysqlUser
	echo -n "password: "
	read -sr mysqlPass

	if [[ ! "$mysqlHost" ]]; then
		mysqlHost="localhost"
	fi

	read -r v1 mysqlVersion <<< $(mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -se "SHOW VARIABLES WHERE variable_name = 'innodb_version'" 2>/dev/null)

	if [[ ${mysqlVersion::1} -gt 5 ]]; then
		v8=1
	fi

	echo -e "\n\n~~ STATUS ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		STATUS" 2>/dev/null

	echo -e "\n\n~~ HOST ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW VARIABLES WHERE variable_name = 'hostname'" 2>/dev/null

	echo -e "\n~~ GLOBAL CHARSET ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW GLOBAL VARIABLES LIKE 'character_set_%';
		SHOW GLOBAL VARIABLES LIKE 'collation%'" 2>/dev/null

	echo -e "\n~~ SESSION CHARSET ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW SESSION VARIABLES LIKE 'character_set_%';
		SHOW SESSION VARIABLES LIKE 'collation%'" 2>/dev/null

	echo -e "\n\n~~ CONNECTIONS ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW VARIABLES WHERE variable_name = 'bind_address';
		SHOW GLOBAL STATUS LIKE '%connect%';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'max_allowed_packet';
		SHOW VARIABLES WHERE variable_name = 'max_connections';
		SHOW STATUS WHERE variable_name = 'Max_used_connections';
		SHOW STATUS WHERE variable_name = 'Max_used_connections_time';
		SHOW VARIABLES WHERE variable_name = 'max_connect_errors';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'wait_timeout'" 2>/dev/null

	echo -e "\n\n~~ THREADS ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW VARIABLES WHERE variable_name = 'thread_cache_size';
		SHOW GLOBAL STATUS WHERE variable_name LIKE 'Threads_%';
		SHOW VARIABLES WHERE variable_name = 'slow_launch_time';
		SHOW STATUS WHERE variable_name ='Slow_launch_threads'" 2>/dev/null

	if [[ "$v8" == 1 ]]; then
		echo -e "\n\n~~ THREADPOOL (v.8 Enterprise) ~~\n"
		mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
			SHOW VARIABLES WHERE variable_name = 'thread_pool_size';
			SHOW VARIABLES WHERE variable_name = 'thread_pool_max_active_query_threads';
			SHOW VARIABLES WHERE variable_name = 'thread_pool_max_unused_threads'" 2>/dev/null
	fi

	echo -e "\n\n~~ TIMEOUTS ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW GLOBAL VARIABLES LIKE '%timeout%'" 2>/dev/null

	echo -e "\n\n~~ OPEN TABLES ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW GLOBAL STATUS LIKE 'Open%tables';
		SHOW VARIABLES WHERE variable_name = 'table_open_cache'" 2>/dev/null

	echo -e "\n\n~~ TEMPORARY TABLES ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW GLOBAL VARIABLES WHERE variable_name LIKE '%_table_size';
		SHOW GLOBAL STATUS WHERE variable_name = 'Created_tmp_tables';
		SHOW GLOBAL STATUS WHERE variable_name = 'Created_tmp_disk_tables';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'temptable_max_ram';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'internal_tmp_mem_storage_engine'" 2>/dev/null

	echo -e "\n\n~~ INNODB KEY VARS ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "

		SHOW GLOBAL VARIABLES WHERE variable_name = 'innodb_strict_mode';
		SHOW VARIABLES WHERE variable_name = 'innodb_file_format';
		SHOW VARIABLES WHERE variable_name = 'innodb_file_per_table';
		SHOW VARIABLES WHERE variable_name = 'innodb_buffer_pool_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_buffer_pool_instances';
		SHOW VARIABLES WHERE variable_name = 'innodb_buffer_pool_chunk_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_change_buffer_max_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_commit_concurrency';
		SHOW VARIABLES WHERE variable_name = 'innodb_thread_concurrency';
		SHOW VARIABLES WHERE variable_name = 'innodb_old_blocks_time';
		SHOW VARIABLES WHERE variable_name = 'innodb_log_buffer_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_log_file_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_page_size';
		SHOW VARIABLES WHERE variable_name = 'innodb_default_row_format';
		SHOW VARIABLES WHERE variable_name = 'innodb_flush_log_at_trx_commit';
		SHOW VARIABLES WHERE variable_name = 'innodb_doublewrite';
		SHOW VARIABLES WHERE variable_name = 'innodb_flush_method';
		SHOW VARIABLES WHERE variable_name = 'innodb_flush_neighbors';
		SHOW VARIABLES WHERE variable_name = 'innodb_io_capacity';
		SHOW VARIABLES WHERE variable_name = 'innodb_io_capacity_max';
		SHOW VARIABLES WHERE variable_name = 'innodb_read_io_threads';
		SHOW VARIABLES WHERE variable_name = 'innodb_write_io_threads'" 2>/dev/null

	echo -e "\n\n~~ INNODB STATS ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW GLOBAL STATUS WHERE variable_name LIKE 'innodb_buffer_%';
		SHOW GLOBAL STATUS WHERE variable_name LIKE 'innodb_data_%';
		SHOW GLOBAL STATUS WHERE variable_name LIKE 'innodb_log_%';
		SHOW GLOBAL STATUS WHERE variable_name LIKE 'innodb_row%';
		SELECT name 'locks', count FROM information_schema.innodb_metrics WHERE name = 'lock_deadlocks' OR name = 'lock_timeouts'" 2>/dev/null

	#echo -e "\n\n~~ INNODB STATUS ~~\n"
	#mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
	#SHOW ENGINE INNODB STATUS\G";

	if [[ "$v8" == 1 ]]; then
		echo -e "\n\n~~ INNODB (v.8) ~~\n"
		mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
			SHOW VARIABLES WHERE variable_name = 'innodb_dedicated_server';
			SHOW VARIABLES WHERE variable_name = 'innodb_log_writer_threads';
			SHOW VARIABLES WHERE variable_name = 'innodb_undo_log_encrypt';
			SHOW VARIABLES WHERE variable_name = 'innodb_redo_log_encrypt'" 2>/dev/null
	fi

	echo -e "\n"

	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SELECT ROUND(100 - (100 * (SELECT Variable_value FROM sys.metrics WHERE Variable_name = 'Innodb_pages_read')
		/ (SELECT Variable_value FROM sys.metrics WHERE Variable_name = 'Innodb_buffer_pool_read_requests')), 2) AS 'Buffer Pool Hit Rate';" 2>/dev/null

	echo -e "\n\n~~ BUFFERS ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW GLOBAL VARIABLES WHERE variable_name = 'sql_buffer_result';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'read_buffer_size';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'read_rnd_buffer_size';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'join_buffer_size';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'sort_buffer_size';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'bulk_insert_buffer_size'" 2>/dev/null

	echo -e "\n\n~~ PERFORMANCE SCHEMA ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW VARIABLES WHERE variable_name = 'performance_schema'" 2>/dev/null

	echo -e "\n\n~~ EVENT SCHEDULER ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW VARIABLES WHERE variable_name = 'event_scheduler'" 2>/dev/null

	echo -e "\n\n~~ QUERY CACHE ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW GLOBAL VARIABLES WHERE variable_name = 'have_query_cache';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'query_cache_type';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'query_cache_size'" 2>/dev/null

	#mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
	#	SHOW GLOBAL STATUS LIKE 'qcache_%';
	#	SHOW GLOBAL VARIABLES LIKE 'query_cache_%'" 2>/dev/null

	echo -e "\n\n~~ REPLICATION ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW VARIABLES WHERE variable_name = 'log_bin';
		SHOW VARIABLES WHERE variable_name = 'sync_binlog';
		SHOW VARIABLES WHERE variable_name = 'binlog_format';
		SHOW VARIABLES WHERE variable_name = 'binlog_encryption';
		SHOW VARIABLES WHERE variable_name = 'log_bin_basename';
		SHOW VARIABLES WHERE variable_name = 'log_bin_index';
		SHOW VARIABLES WHERE variable_name = 'gtid_mode';
		SHOW VARIABLES WHERE variable_name = 'enforce_gtid_consistency';
		SHOW VARIABLES WHERE variable_name = 'server_id';
		SHOW VARIABLES WHERE variable_name = 'relay_log'" 2>/dev/null

	echo -e "\n\n~~ LOGS ~~\n"
	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SHOW VARIABLES WHERE variable_name = 'log_error';
		SHOW VARIABLES WHERE variable_name = 'log_error_verbosity';
		SHOW VARIABLES WHERE variable_name = 'slow_query_log';
		SHOW VARIABLES WHERE variable_name = 'slow_query_log_file';
		SHOW GLOBAL VARIABLES WHERE variable_name = 'long_query_time';
		SHOW GLOBAL STATUS WHERE variable_name = 'slow_queries';
		SHOW VARIABLES WHERE variable_name = 'log_queries_not_using_indexes';
		SHOW VARIABLES WHERE variable_name = 'log_slow_admin_statements';
		SHOW VARIABLES WHERE variable_name = 'log_slow_extra';
		SHOW VARIABLES WHERE variable_name = 'general_log';
		SHOW VARIABLES WHERE variable_name = 'general_log_file'" 2>/dev/null

	#echo -e "\n\n~~ MYISAM KEY VARS ~~\n"
	#mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
	#	SHOW VARIABLES WHERE variable_name = 'myisam_sort_buffer_size';
	#	SHOW VARIABLES WHERE variable_name = 'key_buffer_size'" 2>/dev/null
	#	SELECT
	#		(SELECT variable_value FROM information_schema.global_status WHERE variable_name = 'key_reads') AS key_reads,
	#		(SELECT variable_value FROM information_schema.global_status WHERE variable_name = 'key_read_requests') AS kr_requests,
	#		(SELECT (1 - key_reads / kr_requests) * 100) AS key_cache_hit_percentage";

	#echo -e "\n\n~~ KEYS ~~\n"
	#mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
	#	SHOW GLOBAL STATUS LIKE 'key%'" 2>/dev/null

	echo -e "\n"
}


main "$@"
