SHELL := /bin/bash

#
# compile, upload, monitor
run: upload monitor

#
# monitor defaults are specified in platfomio.ini (monitor_port, monitor_speed)
monitor:
	source ./aliases.sh; dpio device monitor

#
# compile & upload
upload:
	source ./aliases.sh; dpio run -t upload

#
# upload filesystem (unit tests read parameters from unit_tests.json)
uploadfs: data/unit_tests.json
	source ./aliases.sh; dpio run -t uploadfs

clean:
	source ./aliases.sh; dpio run -t clean

list-targets: 
	source ./aliases.sh; dpio run --list-targets

#
# run unit tests
# you can run individual tests with "dpio test -f xxx"
# where xxx is a directory under ./test
#
# be sure to run uploadfs first
tests: 
	source ./aliases.sh; dpio test

