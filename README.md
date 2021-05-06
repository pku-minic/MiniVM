# MiniVM

[![Build and Test](https://github.com/pku-minic/MiniVM/workflows/Build%20and%20Test/badge.svg)](https://github.com/pku-minic/MiniVM)

MiniVM is a virtual machine for interpreting Eeyore/Tigger IR, which is designed for PKU compiler course.

## Building from Source

Before building MiniVM, please make sure you have installed the following dependencies:

* `cmake` 3.13 or later
* C++ compiler supporting C++17
* `flex` and `bison`
* `readline` (optional)

Then you can build this repository by executing the following command lines:

```
$ git clone --recursive https://github.com/pku-minic/MiniVM.git
$ cd MiniVM
$ mkdir build
$ cd build
$ cmake .. && make -j8
```

### Building Without the Built-in Debugger

You can turn the CMake option `NO_DEBUGGER` on to disable the built-in debugger (MiniDbg):

```
$ cmake -DNO_DEBUGGER=ON ..
$ make -j8
```

With this option turned on, you can build MiniVM without `readline`.

## Changelog

See [CHANGELOG.md](CHANGELOG.md)

## License

Copyright (C) 2010-2021 MaxXing. License GPLv3.
