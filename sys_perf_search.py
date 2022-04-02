#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
    Parse and search MySQL Sys and Performance Schema for keyword information.

    Author:  Martin Latter
    Date:    07/02/2022
    Version: 0.03

    Py req:  MySQLdb
             pip3 install mysqlclient

    Usage:   python3 sys_perf_search.py
"""


import sys
import MySQLdb
import MySQLdb.cursors


#################################################################################
# CONFIGURATION
#################################################################################

USER = '<username>'
PASS = '<password>'
HOST = 'localhost'

##

DB = 'performance_schema'    # performance_schema or sys

FIND_WITHIN_TABLES = True    # find within table columns or in table name
TO_FIND = 'USER'             # case-sensitive search term

# see https://github.com/mysql/mysql-sys for sys doc

#################################################################################


def main():

    """ Connect, query, and display results. """

    try:
        conn = MySQLdb.connect(host=HOST, user=USER, passwd=PASS, db=DB)
    except MySQLdb.Error as err:
        print('Cannot connect to database - %d: %s' % (err.args[0], err.args[1]))
        sys.exit(1)

    results = ()
    tables = []

    tables_query = """
        SELECT
            table_name
        FROM
            information_schema.TABLES
        WHERE
            TABLE_SCHEMA = '%s'
        """ % (DB)

    with conn.cursor() as cursor:
        cursor.execute(tables_query)
        results = cursor.fetchall()

    if FIND_WITHIN_TABLES:
        for row in results:
            tables.append(row[0])
    else:
        for row in results:
            if TO_FIND in row[0] and '$' not in row[0]:
                tables.append(row[0])

    print(DB + '\n')

    with conn.cursor() as cursor:
        for table in tables:
            cursor.execute('EXPLAIN ' + table)
            results = cursor.fetchall()

            if FIND_WITHIN_TABLES:
                for row in results:
                    if TO_FIND in row:
                        print(table)
                        for row2 in results:
                            print('\t' + str(row2))
                        print('\n')
            else:
                print(table)
                for row in results:
                    print('\t' + str(row))
                print('\n')

    conn.close()


if __name__ == '__main__':
    main()
