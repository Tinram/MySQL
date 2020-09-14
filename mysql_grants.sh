#!/bin/bash

# mysql_grants.sh
#
# Display MySQL user GRANTs.
# Use an SSH tunnel when connecting to a remote host.

# author      Martin Latter
# copyright   Martin Latter 14/09/2020
# version     0.01
# credits     mleu for Bash example
# license     GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
# link        https://github.com/Tinram/MySQL.git


name="mysql_grants"
version="v.0.01"
date="20200914"
mysqlPort="3306"
scriptName=$0


showHelp()
{
	echo -e "\n$name $version $date\nlist user permissions\n"
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

	if [[ ! $mysqlHost ]]; then
		mysqlHost="localhost"
	fi

	echo -e "\n\n\n~~ GRANTS ~~\n"

	mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "
		SELECT CONCAT('\"', User, '\"@\"', Host, '\"') FROM mysql.user" 2>/dev/null | sort | \

	while read -r user
		do echo -e "\n------"; mysql -u"$mysqlUser" -p"$mysqlPass" -h"$mysqlHost" -P"$mysqlPort" -e "SHOW GRANTS FOR $user" 2>/dev/null | sed 's/$/;/'
	done

	echo -e "\n"
}


main "$@"
