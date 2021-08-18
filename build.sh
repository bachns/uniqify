#!/bin/bash
if [ "$1" = "thread" ]
then
    echo "Build thread version"
    echo "gcc uniqify.c -o msort -std=c11 -Wall -Werror -g3 -O0 -pthread -DTHREAD"
    gcc uniqify.c -o msort -std=c11 -Wall -Werror -g3 -O0 -pthread -DTHREAD
else
    echo "Build process version"
    echo "gcc uniqify.c -o msort -std=c11 -Wall -Werror -g3 -O0"
    gcc uniqify.c -o msort -std=c11 -Wall -Werror -g3 -O0
fi