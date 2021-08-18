#!/bin/bash
if [ $# == 3 ]; then
    FILESIZE=`stat -c%s $2`
    echo "File size, " $FILESIZE "bytes" > $3
    echo "COLUMN_K , COLUMN_TIME" >> $3
    for i in $(seq 1 $1); do
        RTIME=`{ time { ./msort $i < $2 > /dev/null; } > /dev/null; } 2>&1 | grep real | cut -b 6-`
        echo  $i ", " $RTIME >> $3
    done
    echo $0 "is done";
else
    echo $0 "kmax input output.csv"
fi