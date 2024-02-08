#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
    Minimise a mysqldump schema dump.
"""


import argparse
import os
import re
import sys


class SchemaCleaner():

    """
        SchemaCleaner
        Minimise mysqldump schema file dumps, stripping to just table definitions.

        Author         Martin Latter
        Copyright      Martin Latter 06/02/2024
        Version        0.02, from schema_cleaner.php
        License        GNU GPL v3.0
        Link           https://github.com/Tinram/MySQL.git
    """


    # toggle debug
    debug = False

    # schema start table marker - usual for most mysqldump files
    start_table_block = 'DROP TABLE'

    # schema start table marker
    start_table_marker = 'CREATE TABLE'

    # schema end table marker
    end_table_marker = 'saved_cs_client */;'

    # toogle CREATE and DROP statements number match
    match_create_drop_statements = False

    # toggle AUTO_INCREMENT string removal
    remove_auto_increment = True

    # toggle COLLATE definition removal
    remove_collation = True

    schema_filename = 'NONE'
    output_filename = ''
    output_filename_suffix = '_cleaned.sql'
    messages = []
    table_holder = []
    final_table_holder = []


    def __init__(self, file_name):

        """ Initiate all methods. """

        if file_name == '':
            sys.exit('No schema file specified!')
        else:
            self.schema_filename = file_name
            self.output_filename = file_name + self.output_filename_suffix
            self.process_schema()
            self.process_tables()
            self.save_output_file()


    def process_schema(self):

        """ Identify and chop table definitions from schema. """

        try:
            with open(self.schema_filename, 'r',  encoding='UTF8') as schema_file:
                file_contents = schema_file.read()
        except OSError as err:
            sys.exit('Error (' + err + '): ' + self.schema_filename + ' could not be read.')

        # check for start marker presence
        if self.start_table_marker not in file_contents:
            sys.exit('No specified start markers found in ' + self.schema_filename)

        # check for end marker presence
        if self.end_table_marker not in file_contents:
            sys.exit('No specified end markers found in ' + self.schema_filename)

        # check CREATE and DROP statement number match dump file (doesn't work with views, which use DROP TABLE)
        if self.match_create_drop_statements:
            num_drops = file_contents.count(self.start_table_block)
            num_creates = file_contents.count(self.start_table_marker)
            if num_drops != num_creates:
                sys.exit('Error: number of DROP TABLE statements does not match number of CREATE TABLE statements.')

        num_schema_tables = file_contents.count(self.start_table_marker)

        if self.debug:
            num = 1
        else:
            num = num_schema_tables

        i = 0
        offset = 0

        # splice each table block string
        # this method is efficient at removing file text junk compared to regex

        while i < num:

            if offset == 0:
                start = file_contents.index(self.start_table_marker)
                end = file_contents.index(self.end_table_marker, start)
            else:
                start = file_contents.index(self.start_table_marker, end)
                end = file_contents.index(self.end_table_marker, start)

            frag = end + (len(self.end_table_marker))
            table = file_contents[start : frag]

            self.table_holder.append(table)

            offset = end
            i += 1


    def process_tables(self):

        """ Process each table block. """

        rx_create_table = r'CREATE TABLE .+ \('
        rx_pk = r'.+PRIMARY KEY'
        i = 0

        for table in self.table_holder:

            start = table.index(self.start_table_marker)
            end = table.index(self.end_table_marker, start)

            tmp = end + len(self.end_table_marker)
            table = table[start : tmp]

            start = table.index('/*!')
            table = table[0 : start]

            # remove autoincrement string
            if self.remove_auto_increment:
                table = re.sub(' AUTO_INCREMENT=[0-9]+', '', table)

            # remove collate strings
            if self.remove_collation:
                table = re.sub(r'COLLATE [\w]+ ', '', table)
                table = re.sub(r'COLLATE=[\w]+;', ';', table)

            # add whitespace to improve readability
            table = table.replace(') ENGINE', '\n) ENGINE')
            match = re.search(rx_create_table, table)
            table = re.sub(rx_create_table, f'{match.group()}' + "\n", table)
            match = re.search(rx_pk, table)
            try:
                table = re.sub(rx_pk, "\n" + f'{match.group()}' , table)
            except AttributeError:
                pass # ignore group error
            table = table.replace(' ;', ';')

            # capitalise data types
            types = ['int', 'tinyint', 'smallint', 'mediumint', 'bigint', 'float', 'double', 'decimal', 'varchar', 'char', 'text', 'tinytext', 'mediumtext', 'longtext', 'blob', 'mediumblob', 'longblob', 'binary', 'varbinary', 'datetime', 'date', 'timestamp', 'time', 'year', 'bit', 'boolean', 'enum', 'unsigned']

            for typ in types:
                table = table.replace(' ' + typ, ' ' + typ.upper())

            # separate table blocks with whitespace
            if not i:
                table = "\n" + table
                i += 1

            table = table + "\n\n\n"

            self.final_table_holder.append(table)


    def save_output_file(self):

        """ Save table definitions into single file. """

        with open(self.output_filename, 'w',  encoding='UTF8') as out_file:
            tables = ''.join(self.final_table_holder)
            out_file.write(tables)
            print(self.output_filename + ' saved.')


def main():

    """ Process file argument and check file is present and readable. """

    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--file', dest='filename', help='parse file named FILENAME', type=str, action='store', required=True)
    args = parser.parse_args()
    file_name = args.filename

    # check file access
    if not os.path.isfile(file_name):
        sys.exit('Error: ' + file_name + ' does not exist in this directory.')
    elif not os.access(file_name, os.R_OK):
        sys.exit('Error: ' + file_name + ' is not readable (check file attributes).')
    else:
        SchemaCleaner(file_name)


if __name__ == '__main__':
    main()
