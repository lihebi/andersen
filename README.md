# Andersen points-to analysis

## handled cases
```
a=b;
a=&v;
*p=q;
p=*q;
```

## unhandled
* library call
* global variable
* inter-procedure

# usage

```
cmake .
make
```

# test
Compile program using

```
./configure CC=clang
make CC=clang CFLAGS="-g -O0 -emit-llvm"
```

Run pass:

```
opt -load hellopass/libAndersenPass.so -andersen test.bc >/dev/null
```

Or using the script to run on all bc files provided as cmd arguments:

```
./test-all benchmark/grep/*.bc
```
