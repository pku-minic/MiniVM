#ifndef MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_
#define MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_

#include <string_view>
#include <string>
#include <fstream>
#include <unordered_map>
#include <cstdint>

class SourceReader {
 public:
  SourceReader(std::string_view src_file) : ifs_(std::string(src_file)) {}

  // read the content of the specific line number in source file
  // undefined when 'line_num' is out of range
  std::string_view ReadLine(std::uint32_t line_num);

 private:
  // goto the specific line number in file
  void SeekLine(std::uint32_t line_num);

  // stream of source file
  std::ifstream ifs_;
  // buffer of all read lines
  std::unordered_map<std::uint32_t, std::string> lines_;
};

#endif  // MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_
