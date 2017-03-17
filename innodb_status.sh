#!/bin/bash

# innodb_status.sh
# display a shorter/tidier InnoDB monitor output
# beware the variable time average 'Per second averages calculated from the last XX seconds' under the first heading

# author      Martin Latter <copysense.co.uk>
# version     0.01
# link        https://github.com/Tinram/MySQL.git


mysql -e "SHOW ENGINE INNODB STATUS" -u root -p | awk '{gsub(/\\n/, "\n")}1'
