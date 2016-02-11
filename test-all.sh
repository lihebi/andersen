#!/bin/bash

for fname in $@
do
    echo "=========== Analyzing $fname .."
    opt -load hellopass/libAndersenPass.so -andersen $fname >/dev/null
done

# for fname in $1
# do
#     # opt -load hellopass/libAndersenPass.so -andersen $fname >/dev/null
# done

# for fname in $1/*.o
# do
#     echo "Analyzing $fname .."
#     # opt -load hellopass/libAndersenPass.so -andersen $fname >/dev/null
# done
