# MiniVM

[![Build and Test](https://github.com/pku-minic/MiniVM/workflows/Build%20and%20Test/badge.svg)](https://github.com/pku-minic/MiniVM)

MiniVM is a virtual machine for interpreting Eeyore/Tigger IR, which is designed for PKU compiler course.

## Building from Source

Before building MiniVM, please make sure you have installed the following dependencies:

* `cmake` 3.13 or later
* C++ compiler supporting C++17
* `readline`

Then you can build this repository by executing the following command lines:

```
$ git clone --recursive https://github.com/pku-minic/MiniVM.git
$ cd MiniVM
$ mkdir build
$ cd build
$ cmake .. && make -j8
```

## License

Copyright (C) 2010-2021 MaxXing. License GPLv3.
