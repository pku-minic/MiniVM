#ifndef MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_
#define MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_

#include <string_view>
#include <string>
#include <fstream>
#include <unordered_map>
#include <cstdint>

// source file reader
class SourceReader {
 public:
  SourceReader(std::string_view src_file) : ifs_(std::string(src_file)) {
    InitTotalLines();
  }

  // read the content of the specific line number in source file
  // returns empty string when 'line_num' is out of range
  std::string_view ReadLine(std::uint32_t line_num);

 private:
  // initialize total line number
  void InitTotalLines();
  // goto the specific line number in file
  void SeekLine(std::uint32_t line_num);

  // stream of source file
  std::ifstream ifs_;
  // total line number
  std::uint32_t total_lines_;
  // buffer of all read lines
  std::unordered_map<std::uint32_t, std::string> lines_;
};

#endif  // MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_
