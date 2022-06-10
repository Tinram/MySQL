#!/usr/bin/env python3

# 11/05/2022

import csv
from matplotlib import pyplot as plt
from pylab import *
csvFile = 'trx.log'

data = []


with open(csvFile, 'r') as f:

    reader = csv.DictReader(f, delimiter='|')
    #print(reader.fieldnames)

    for row in reader:
        data.append(row['wait'])


plt.plot(data,label='trx wait (s)')
plt.title('trx test plot')
plt.ylabel('time (s)')
plt.xlabel('trx')
plt.legend()
plt.grid(True,color='#f1f1f1')
plt.show()