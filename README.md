# pdc

This project is written in C, but it's using my fork of Clang
which adds a bunch of syntax changes to the language.

```
git clone https://github.com/peterdelevoryas/llvm-project
cd llvm-project
mkdir build
cd build
cmake -G Ninja -DLLVM_ENABLE_PROJECTS=clang ../llvm
ninja clang
cd ../..
git clone https://github.com/peterdelevoryas/pdc
cd pdc
mkdir build
cd build
cmake -G Ninja -DCMAKE_C_COMPILER=../../llvm-project/build/bin/clang ..
ninja
./pdc ../test.pd ../libc.pd
./a.out
```
