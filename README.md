# C-compiler
C to LLVM / Python compiler

# Configure Environment

1. download from [llvm-3.8.0](http://llvm.org/releases/3.8.0/llvm-3.8.0.src.tar.xz)
2. ```tar xf llvm-3.8.0.src.tar.xz```
3. ```cd llvm-3.8.0.src.tar.xz```
4. ```mkdir build```
5. ```cd build```
6. ```../configure```
7. ```make```
8. ```make install```

*Note that make will take about half an hour*

# Compile
To compile the compiler source code  

1. ```make clean```
2. ```make```

# Run code

- ```./compiler [-vfph] <c_code/your_code.c>```

- -v: display generated back-end code in the terminal.
- -f: output generated back-end code to ```<c_code/your_code.[(ll)/(py)]>```.
- -p: alternatively generate python code.
- -h: help

# Test code

- using ```lli c_code/your_code.ll``` to test llvm back-end code.
- using ```python c_code/your_code.py``` to test python back-end code.
