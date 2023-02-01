#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
    Parse and search MySQL Sys and Performance Schema
    for keywords in table names and table fields.

    Author:  Martin Latter
    Date:    07/02/2022
    Version: 0.05

    Py req:  MySQLdb
             pip3 install mysqlclient

    Usage:   python3 sys_perf_search.py
"""


import sys
import MySQLdb


#################################################################################
# CONFIGURATION
#################################################################################

USER = '<username>'
PASS = '<password>'
HOST = '<host>'

##

DB = 'performance_schema'
# performance_schema | sys | information_schema

KEYWORD = 'transaction'
# case-insensitive search term


""" sys documentation: https://github.com/mysql/mysql-sys """

#################################################################################


def main():

    """ Connect, query, and display results. """

    try:
        conn = MySQLdb.connect(host=HOST, user=USER, passwd=PASS, db=DB)
    except MySQLdb.Error as err:
        print('Cannot connect to database - %d: %s' % (err.args[0], err.args[1]))
        sys.exit(1)

    results = ()
    table_fields = []
    table_names = []

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

    for row in results:
        if 'x$' not in row[0]:
            table_fields.append(row[0])
            if KEYWORD.lower() in row[0].lower():
                table_names.append(row[0])

    print('\n' + str.upper(DB) + '\n\n\t' + 'search: ' + KEYWORD + '\n\n')

    with conn.cursor() as cursor:
        for table in table_fields:
            cursor.execute('EXPLAIN ' + table)
            results = cursor.fetchall()
            for row in results:
                if KEYWORD.lower() in row[0].lower():
                    print('\n' + table + '\n')
                    for row in results:
                        print('\t' + str(row))
                    print('\n')
                    break

        if table_names:
            print('\ntable names:\n')
            for table in table_names:
                print('\t' + table + '\n')

    conn.close()


if __name__ == '__main__':
    main()
