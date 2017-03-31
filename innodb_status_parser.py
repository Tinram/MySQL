#!/usr/bin/python
# -*- coding: utf-8 -*-

"""
    InnoDB status output parser.
"""



## CONFIGURATION ##

CONFIG = {

    'USERNAME':     'root',
    'HOST':         'localhost',

    'USE_FILE':     False,                  # toggle: False = connect to mysqld; True = parse text file of InnoDB output
    'FILENAME':     'example.txt'           # local file to parse
}



import sys
import subprocess
import re


class InnodbStatusParser:

    """
        InnodbStatusParser
        Parse MySQL's 'INNODB SHOW ENGINE STATUS' output.

        Python Version       2.6 to 3.6
        Python 3 Usage       Replace shebang with: #!/usr/bin/python3 or call script with python3 <script_name>

        Author               Martin Latter <copysense.co.uk>
        Copyright            Martin Latter 22/03/17
        Version              0.11
        License              GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
        Link                 https://github.com/Tinram/MySQL.git
    """


    buffer_pool_size = []
    fk_errors = []
    log_sequence = []
    log_flush = []
    output = []
    queries_inside = []
    queries_queue = []
    rows = []
    semaphores = []
    time_period = []
    timestamp = []
    total_memory = []
    transaction_locks = []


    def __init__(self):

        self.acquire_data()
        self.process_data()
        self.display_data()


    def acquire_data(self):

        """ Acquire the mysqld output or local file data. """

        if not CONFIG['USE_FILE']:

            cmd = subprocess.Popen (

                'mysql -e "SHOW ENGINE INNODB STATUS" -h ' + CONFIG['HOST'] + ' -u ' + CONFIG['USERNAME'] + ' -p',
                shell = True,
                stdout = subprocess.PIPE
            )

            self.output = cmd.stdout

        else:

            if CONFIG['USE_FILE']:

                self.spacer()
                print('parsing local file "' + CONFIG['FILENAME'] + '"')

            try:

                with open(CONFIG['FILENAME'], 'r') as locfile:

                    filecontents = locfile.readlines()

            except IOError as err:

                if err.errno == 2:
                    print('FILE ISSUE: "' + CONFIG['FILENAME'] + '" could not be found in this directory!\n')
                else:
                    print(err)

                sys.exit(1)

            else: # harmonise between file pipe and local file parsing for regex parsing

                tmp = ' '.join(filecontents)
                self.output.append(tmp)
                self.output.append(' ')

    # end acquire_data()


    def process_data(self):

        """ Process the data. """

        for line in self.output:

            if not CONFIG['USE_FILE']:
                currline = str(line)
            else:
                currline = line.replace('\r\n', ' ')
                currline = line.replace('\n', ' ')

            # parse InnoDB information via regex

            tstmp = re.findall(r'(\d{4}\-\d{2}\-\d{2} \d{2}\:\d{2}\:\d{2}) .+ INNODB MONITOR OUTPUT', currline)
            if tstmp:
                self.timestamp = tstmp

            tpd = re.findall('last (\d+) seconds', currline)
            if tpd:
                self.time_period = tpd

            tmem = re.findall(r'Total memory allocated (\d+)', currline)
            if tmem:
                self.total_memory = tmem

            quin = re.findall(r'(\d+) queries inside', currline)
            if quin:
                self.queries_inside = quin

            ququ = re.findall(r'(\d+) queries in queue', currline)
            if ququ:
                self.queries_queue = ququ

            rws = re.findall(r'rows inserted (\d+), updated (\d+), deleted (\d+), read (\d+)', currline)
            if rws:
                self.rows = rws

            bps = re.findall(r'Buffer pool size\s+(\d+)', currline)
            if bps:
                self.buffer_pool_size = bps

            lsn = re.findall(r'Log sequence number\s+(\d+)', currline)
            if lsn:
                self.log_sequence = lsn

            lfs = re.findall(r'Log flushed up to\s+(\d+)', currline)
            if lfs:
                self.log_flush = lfs

            sem = re.findall(r'--Thread .+ seconds', currline)
            if sem:
                self.semaphores.append(' | ' . join(sem))

            fke = re.findall(r'LATEST FOREIGN KEY ERROR .+ there is a record:', currline)
            if fke:
                self.fk_errors.append('' . join(fke))

            tlk = re.findall(r'LOCK WAIT .+ RECORD LOCKS', currline)
            if tlk:
                self.transaction_locks.append(tlk)

    # end process_data()


    def display_data(self):

        """ Display the results. """

        self.spacer()
        self.header()
        self.title('INNODB STATUS PARSER')
        self.header()
        self.sep()

        if self.timestamp:

            self.title('REPORT TIMESTAMP')
            print('\t' + '' . join(self.timestamp))
            self.sep()

        self.title('COLLECTION TIME PERIOD')
        print('data collected in: ' + ' ' . join(self.time_period) + ' seconds')
        self.sep()

        if self.rows:

            self.title('ROW DATA')
            print('since server started/restarted:\n')
            print('rows inserted: ' + self.rows[0][0])
            print('rows updated: ' + self.rows[0][1])
            print('rows deleted: ' + self.rows[0][2])
            print('rows read: ' + self.rows[0][3])
            self.sep()

        self.title('ROW OPERATIONS')
        print('queries inside InnoDB: ' + ' ' . join(self.queries_inside))
        print('queries in queue: ' + ' ' . join(self.queries_queue))
        self.sep()

        if self.buffer_pool_size:

            self.title('BUFFER POOL SIZES')
            print('\n' . join(self.buffer_pool_size))
            print('\n(in pages)')
            self.sep()

        if self.log_flush != self.log_sequence:

            self.title('UNFLUSHED DATA')
            diff = ( int(self.log_sequence[0]) - int(self.log_flush[0]) ) / 1024
            diff2 = '%.4f' % diff
            print('\n' + diff2 + ' kB data is unflushed to disk.')
            self.sep()

        if self.total_memory:

            self.title('TOTAL MEMORY')
            print('total memory: ' + ' ' . join(self.total_memory) + ' bytes')
            self.sep()

        if self.semaphores:

            self.title('THREADS')
            print('thread waits:\n')
            print(' ' . join(self.semaphores))
            self.sep()

        if self.transaction_locks:

            self.title('TRANSACTION LOCKS')

            for tlocks in self.transaction_locks:
                # clean up strings
                tmp = str(tlocks)
                tmp = tmp.replace('\\\'', '\'')
                tmp = tmp.replace('LOCKS', 'LOCKS\n\n')
                print(tmp)

            self.sep()

        if self.fk_errors:

            self.title('FOREIGN KEY ERRORS')
            print(' ' . join(self.fk_errors))
            self.sep()

    # end display_data()


    def spacer(self):

        """ Print empty line. """

        print('\n')


    def sep(self):

        """ Print separator line. """

        print('\n' + ('_' * 50) + '\n')


    def header(self):

        """ Print header line. """

        print('*' * 50)


    def title(self, text):

        """ Create title character fluff and print title. """

        strlen = len(text)
        seplen = 16

        while (strlen + (seplen * 2)) > 35:
            seplen = seplen - 1

        titleline = ' ' + ('-' * seplen) + ' '
        titleout = '%s%s%s\n' % (titleline, text, titleline)

        print(titleout)

# end class



def main():

    """ Invoke class. """

    InnodbStatusParser()


if __name__ == '__main__':

    main()
