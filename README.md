# Andersen points-to analysis

## handled cases
* Four standard andersen statements
  * a=b;
  * a=&v;
  * *p=q;
  * p=*q;
* handle arbitrary level pointer dereference

## unhandled
* library call
* global variable
* inter-procedure
* structure
* array

# compiling
Compiled successfully with latest llvm version (3.9.0).
All the tests are done under CentOS 6.
Compile and install LLVM, then switch to this directory and run:

```
cmake .
make
```

This will generate `hellopass/libAndersenPass.so`.

# test

## simple test
`./benchmark/pointer.c` contains some test statement that contains all four standard andersen statements, as well multiple dimension pointer dereference.
To test, run the following scripts in this directory:

```sh
./compile-simple.sh # will compile benchmark/pointer.c into ./pointer.bc
./test-simple.sh # will analyze pointer.bc and output to output.txt
```

## real benchmarks
There're two benchmarks in `benchmark` folder: gawk-4.1.3 and make-4.1.

To compile benchmark program under llvm(use gawk as example):

```sh
tar zxvf gawk-4.1.3.tar.gz
cd gawk-4.1.3
./configure CC=clang
make CC=clang CFLAGS="-g -O0 -emit-llvm"
```

Test all generated bitcode files using scripts in this directory:

```sh
./test-real benchmark/gawk-4.1.3/*.o
```

This will output to `output.txt`.

# experiment result on real benchmarks
For gawk, the result is:

```
builtin.c:668 cur ===> {fw, prec, }
getopt.c:544 ambig_list ===> {first, }
io.c:3728 isi.addr ===> {buf, }
io.c:1435 lres ===> {lhints, }
io.c:1435 lres0 ===> {lhints, }
./regcomp.c:3925 p_new ===> {dup_root, }
./regexec.c:1059 err.addr.i ===> {err, }
./regexec.c:1428 fs ===> {fs_body, }
```

For make, the result is:

```
function.c:936 wordtail ===> {wordhead, }
function.c:939 pattail ===> {pathead, }
implicit.c:577 dptr ===> {dl, }
read.c:3017 newp ===> {new, }
read.c:3055 nlist ===> {name, }
remake.c:422 ad ===> {amake, }
strcache.c:80 spp ===> {sp, }
```
