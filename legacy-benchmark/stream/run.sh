#!/bin/bash
TIME=`date +%s`

./stream_c.exe | grep "^Copy\:" | awk '{print $2" "$3" "$4" "$5}' >> data/s.$TIME
