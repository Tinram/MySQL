#!/usr/bin/env python3

"""
    Plot example.

    Author         Martin Latter
    Copyright      Martin Latter 11/05/2022
    Version        0.02
    License        GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
    Link           https://github.com/Tinram/MySQL.git
"""


import csv
from matplotlib import pyplot as plt


CSV_FILE = 'trx.csv'
DATA = []

with open(CSV_FILE, encoding='utf-8', mode='r') as f:

    READER = csv.DictReader(f, delimiter='|')
    #print(READER.fieldnames)

    for row in READER:
        DATA.append(row['wait'])


plt.plot(DATA, label='trx wait (s)')
plt.title('trx test plot')
plt.ylabel('time (s)')
plt.xlabel('trx')
plt.legend()
plt.grid(True, color='#f1f1f1')
plt.show()
