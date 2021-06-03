# Changelog

All notable changes to the MiniVM will be documented in this file.

## Unreleased

### Added

* Detection about functions without `return` statements.

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
