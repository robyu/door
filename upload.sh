#!/bin/bash

#
# create a file called webapi_url.txt
# which contains the iftt webhook URL
# be sure to escape all backslashes, e.g.
#
# \/trigger\/door_sensor\/with\/key\/abcde1234
#
webapi_url=`cat webapi_url.txt`
echo "$webapi_url"

#
# create tx.h
sed 's/$webhook_url/'"$webapi_url"'/' < src/tx_template.h > src/tx.h

#
# compile and upload
platformio run -e nodemcuv2 --target upload --upload-port=/dev/tty.SLAB_USBtoUART

