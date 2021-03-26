#include "debugger/minidbg/srcreader.h"

#include <limits>
#include <utility>
#include <cassert>

void SourceReader::SeekLine(std::uint32_t line_num) {
  ifs_.seekg(std::ios::beg);
  for (std::uint32_t i = 0; i < line_num - 1; ++i) {
    ifs_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
}

std::string_view SourceReader::ReadLine(std::uint32_t line_num) {
  // try to find in buffer
  auto it = lines_.find(line_num);
  if (it != lines_.end()) {
    return it->second;
  }
  else {
    // read line from file
    SeekLine(line_num);
    std::string line;
    std::getline(ifs_, line);
    assert(ifs_);
    // add to buffer
    auto ret = lines_.insert({line_num, std::move(line)});
    assert(ret.second);
    return ret.first->second;
  }
}
