#!/bin/bash

# test Andersen Pass

# opt -load hellopass/libAndersenPass.so -andersen <pointer.bc >/dev/null
opt -load hellopass/libAndersenPass.so -andersen pointer.bc >/dev/null
