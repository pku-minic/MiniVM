# Changelog

All notable changes to the MiniVM will be documented in this file.

## 0.2.1 - 2021-12-03

### Changed

* MiniVM will disrupt all local allocated memories.
* MiniVM will not zero initialize Tigger registers.

## 0.2.0 - 2021-10-14

### Added

* C backend for converting Eeyore/Tigger programs into C programs.

### Fixed

* Bug about reading/writing `x0` register in Tigger mode.

## 0.1.4 - 2021-10-13

### Added

* Detection about functions without `return` statements.

### Changed

* MiniVM will not perform initialization in the local allocation, but will still initialize all global memory.
* Tigger library functions will disrupt all caller-saved registers.

### Fixed

* Problem about printing error message for the `x` command in MiniDbg.

## 0.1.3 - 2021-05-12

### Fixed

* Problem about parsing non-existent files.
* Bug about handling function return values.

## 0.1.2 - 2021-05-06

### Added

* Supported `NO_DEBUGGER` mode to disable the built-in debugger (MiniDbg).

### Fixed

* Made GCC/G++ happy.

## 0.1.1 - 2021-04-08

### Fixed

* Problem about the line number in front end error message.

## 0.1.0 - 2021-04-03

### Added

* The brand new debugger: MiniDbg.

### Changed

* Supported returning error code when VM performs an error.

### Fixed

* Bug in `Break` instruction.

## 0.0.1 - 2020-12-21
