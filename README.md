# MiniVM

MiniVM is a virtual machine for interpreting Eeyore/Tigger IR, which is designed for PKU compiler course.

## Building from Source

Before building MiniVM, please make sure you have installed the following dependencies:

* `cmake` 3.5 or later
* C++ compiler supporting C++17

Then you can build this repository by executing the following command lines:

```
$ git clone --recursive https://github.com/MaxXSoft/MiniVM.git
$ cd MiniVM
$ mkdir build
$ cd build
$ cmake .. && make -j8
```

## License

Copyright (C) 2010-2020 MaxXing. License GPLv3.
