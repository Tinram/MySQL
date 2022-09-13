#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
    Bulk Loader

    Bulk load CSV file into new or existing InnoDB table via a temporary table.
    ~2x faster than LOAD DATA direct into InnoDB table.

    Author:     Martin Latter
    Date:       07/09/2022
    Version:    0.04
    License:    GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
    Link:       https://github.com/Tinram/MySQL.git

    Py req:     MySQLdb
                pip3 install mysqlclient
"""


import contextlib
import sys
import time

import MySQLdb
import MySQLdb.cursors


#############################################################
# CONFIGURATION
#############################################################

CREATE_FINAL_TABLE = True

DATAFILE = 'junk.csv'
TMP_TABLE_NAME = 'mem_import'
FINAL_TABLE_NAME = 'users'

CREDENTIALS_DB_LOCAL = dict(
    user   = 'root',
    passwd = '',
    host   = 'localhost',
    db     = 'bulkload',
    port   = 3306,
    cursorclass=MySQLdb.cursors.DictCursor
)

TEMP_TABLE = """
CREATE TEMPORARY TABLE IF NOT EXISTS `%s`(
    `id`           INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `firstname`    VARCHAR(20)  NOT NULL,
    `lastname`     VARCHAR(20)  NOT NULL,
    `country`      VARCHAR(20)  NOT NULL DEFAULT '',
    `country_code` CHAR(2)      NOT NULL,
    KEY `idx_fl` (`firstname`, `lastname`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4;
""" % (TMP_TABLE_NAME)

FIELD_NAMES = 'firstname, lastname, country, country_code'


"""
server settings to consider adjusting:
    log_bin = OFF
    innodb_doublewrite = OFF
    innodb_flush_method = O_DIRECT
    innodb_change_buffer_max_size = 50
    ALTER INSTANCE disable innodb redo_log; # 8.0.21+
"""

#############################################################


UK = """ SET SESSION unique_checks = OFF """
FK = """ SET SESSION foreign_key_checks = OFF """
FL = """ SET GLOBAL innodb_flush_log_at_trx_commit = 2 """
IF = """ SET GLOBAL local_infile = ON """
IFO = """ SET GLOBAL local_infile = OFF """ # attempt some security


IMPORT = """
LOAD DATA LOCAL INFILE '%s' INTO TABLE `%s` FIELDS TERMINATED BY ',' IGNORE 1 LINES (%s)
""" % (DATAFILE, TMP_TABLE_NAME, FIELD_NAMES)

CONV = """
ALTER TABLE `%s` ENGINE=InnoDB
""" % (TMP_TABLE_NAME)

CT = """
CREATE TABLE `%s` LIKE `%s`
""" % (FINAL_TABLE_NAME, TMP_TABLE_NAME)

if CREATE_FINAL_TABLE:
    INS = """
    INSERT INTO `%s` SELECT * FROM `%s`;
    """ % (FINAL_TABLE_NAME, TMP_TABLE_NAME)
else:
    INS = """
    INSERT INTO `%s` (%s) SELECT %s FROM `%s`;
    """ % (FINAL_TABLE_NAME, FIELD_NAMES, FIELD_NAMES, TMP_TABLE_NAME)

DEL = """
DROP TABLE `%s`
""" % (TMP_TABLE_NAME)


#################################################################################


def main():

    """ Connect and run queries. """

    try:
        conn = MySQLdb.connect(**CREDENTIALS_DB_LOCAL)
    except MySQLdb.Error as err:
        print('Cannot connect to database - %d: %s' % (err.args[0], err.args[1]))
        sys.exit(1)

    with contextlib.closing(conn):

        with conn.cursor() as cursor:

            try:
                cursor.execute(IF)
            except MySQLdb.Error as err:
                print(err)

            try:
                cursor.execute(UK)
            except MySQLdb.Error as err:
                print(err)

            try:
                cursor.execute(FK)
            except MySQLdb.Error as err:
                print(err)

            try:
                cursor.execute(IF)
            except MySQLdb.Error as err:
                print(err)

            try:
                cursor.execute(TEMP_TABLE)
            except MySQLdb.Error as err:
                print(err)

            try:
                if CREATE_FINAL_TABLE:
                    print('adding to new table ...')
                else:
                    print('adding to existing table ...')
                start = time.time()
                imp = cursor.execute(IMPORT)
                if imp > 0:
                    print(str(imp) + ' rows loaded')
                finish = time.time() - start
                print('LOAD DATA: ' + str(round(finish, 3)) + 's')
            except MySQLdb.Error as err:
                print(err)

            if CREATE_FINAL_TABLE:
                try:
                    cursor.execute(CONV)
                except MySQLdb.Error as err:
                    print(err)

                try:
                    cursor.execute(CT)
                except MySQLdb.Error as err:
                    print(err)

            try:
                cursor.execute(INS)
            except MySQLdb.Error as err:
                print(err)

            try:
                cursor.execute(DEL)
            except MySQLdb.Error as err:
                print(err)

            try:
                cursor.execute(IFO)
            except MySQLdb.Error as err:
                print(err)

if __name__ == '__main__':
    main()
