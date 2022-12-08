#!/bin/bash

# db_copy.sh
#
# Copy a MySQL database between servers via mysqlpump/mysqldump and PC sink, preserving charset and collations.
# Avoids the human time of manual mysqldump export file and reload, plus charset amnesia for migration.
# Created to circumvent the flaky operation of mysql-utilities:mysqldbcopy 1.6.1 (which should do all of this).
#
# author      Martin Latter
# copyright   Martin Latter 07/12/2022
# version     0.02
# license     GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
# link        https://github.com/Tinram/MySQL.git


NAME="db_copy"
VERSION="v.0.02"
DATE="20221208"
UTILITY="mysqlpump" # replace with 'mysqldump' if mysqlpump unavailable (MariaDB)
MYSQLPUMP_THREADS=4
PORT="3306"
SCRIPTNAME=$0


showHelp()
{
	echo -e "\n$NAME $VERSION $DATE\ncopy MySQL database between servers via mysqlpump and PC sink\n"
	echo -e "usage: $SCRIPTNAME\n"
}


main()
{
	if [[ $1 == "-h" || $1 == "--help" ]]; then
		showHelp
		exit 1
	fi

	echo -e "\n$NAME $VERSION\n"

	##

	echo -n "destination host: (blank = localhost) "
	read -r DST_HOST

	if [[ ! "$DST_HOST" ]]; then
		DST_HOST="localhost"
	fi

	echo -n "destination username: "
	read -r DST_USER
	if [[ ! "$DST_USER" ]]; then
		echo "no destination username entered ... aborting"
		exit 1
	fi

	echo -n "destination server password: "
	read -sr DST_PASSWORD

	##
	echo -e "\n"

	echo -n "source host: (blank = localhost) "
	read -r SRC_HOST

	if [[ ! "$SRC_HOST" ]]; then
		SRC_HOST="localhost"
	fi

	echo -n "source username: "
	read -r SRC_USER
	if [[ ! "$SRC_USER" ]]; then
		echo 'no source username entered ... aborting'
		exit 1
	fi

	echo -n "source server password: "
	read -sr SRC_PASSWORD

	echo -e "\n"

	echo -n "source database: "
	read -r SRC_DB
	if [[ ! "$SRC_DB" ]]; then
		echo "no source database name given ... aborting"
		exit 1
	fi

	##

	echo -e "\nsource database size"
	read -r SRC_DB_SIZE <<< $(mysql -u"$SRC_USER" -p"$SRC_PASSWORD" -h"$SRC_HOST" -P"$PORT" -se "SELECT CONCAT(SUM(ROUND(((DATA_LENGTH - (INDEX_LENGTH + DATA_FREE)) / 1024 / 1024), 2)), ' MB') ssize FROM information_schema.TABLES WHERE TABLE_SCHEMA = '$SRC_DB';" 2>/dev/null)
	echo -e "$SRC_DB_SIZE\n"

	read -r CHARSET COLLATION <<< $(mysql -u"$SRC_USER" -p"$SRC_PASSWORD" -h"$SRC_HOST" -P"$PORT" -se "SELECT DEFAULT_CHARACTER_SET_NAME, DEFAULT_COLLATION_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = '$SRC_DB';" 2>/dev/null)

	echo -e "creating destination database ...\n"

	mysql -u"$DST_USER" -p"$DST_PASSWORD" -h"$DST_HOST" -P"$PORT" -e "CREATE DATABASE IF NOT EXISTS \`$SRC_DB\` CHARSET=$CHARSET COLLATE=$COLLATION;" 2>/dev/null

	echo -e "copying source database to destination ...\n"

	if [[ "$UTILITY" == "mysqlpump" ]]; then
		mysqlpump -u"$SRC_USER" -p"$SRC_PASSWORD" -h"$SRC_HOST" -P"$PORT" --single-transaction --no-create-db --set-gtid-purged=OFF --compress --watch-progress --default-parallelism="$MYSQLPUMP_THREADS" "$SRC_DB" 2>/dev/null | mysql -u"$DST_USER" -p"$DST_PASSWORD" -h"$DST_HOST" -P"$PORT" "$SRC_DB" 2>/dev/null
			# --column-statistics=0 # avoid error 1109; newer versions of mysqlpump
	else
		mysqldump -u"$SRC_USER" -p"$SRC_PASSWORD" -h"$SRC_HOST" -P"$PORT" --single-transaction --set-gtid-purged=OFF --compress -v "$SRC_DB" 2>/dev/null | mysql -u"$DST_USER" -p"$DST_PASSWORD" -h"$DST_HOST" -P"$PORT" "$SRC_DB" 2>/dev/null
			# --column-statistics=0 # avoid error 1109; newer versions of mysqldump
			# --quick               # huge DBs
	fi

	echo -e "\ndestination database size"
	read -r DST_DB_SIZE <<< $(mysql -u"$DST_USER" -p"$DST_PASSWORD" -h"$DST_HOST" -P"$PORT" -se "SELECT CONCAT(SUM(ROUND(((DATA_LENGTH - (INDEX_LENGTH + DATA_FREE)) / 1024 / 1024), 2)), ' MB') dsize FROM information_schema.TABLES WHERE TABLE_SCHEMA = '$SRC_DB';" 2>/dev/null)
	echo -e "$DST_DB_SIZE\n"
}


main "$@"
