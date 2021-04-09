#ifndef MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_
#define MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_

#include <optional>
#include <string_view>
#include <string>
#include <fstream>
#include <unordered_map>
#include <cstdint>

namespace minivm::debugger::minidbg {

// source file reader
class SourceReader {
 public:
  SourceReader(std::string_view src_file) : ifs_(std::string(src_file)) {
    InitTotalLines();
  }

  // read the content of the specific line number in source file
  // returns 'nullopt' when 'line_num' is out of range
  std::optional<std::string_view> ReadLine(std::uint32_t line_num);

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

}  // namespace minivm::debugger::minidbg

#endif  // MINIVM_DEBUGGER_MINIDBG_SRCREADER_H_
