#include "vmconf.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include "front/token.h"
#include "mem/sparse.h"
#include "mem/dense.h"

using namespace minivm::vm;
using namespace minivm::mem;
using namespace minivm::front;

// add all library functions to a VM instance
#define ADD_LIBS(vm)                                     \
  do {                                                   \
    vm.RegisterFunction("f_getint", GetInt);             \
    vm.RegisterFunction("f_getch", GetCh);               \
    vm.RegisterFunction("f_getarray", GetArray);         \
    vm.RegisterFunction("f_putint", PutInt);             \
    vm.RegisterFunction("f_putch", PutCh);               \
    vm.RegisterFunction("f_putarray", PutArray);         \
    vm.RegisterFunction("f__sysy_starttime", StartTime); \
    vm.RegisterFunction("f__sysy_stoptime", StopTime);   \
  } while (0)

namespace {
// implementation of SysY library functions

// core implementations
namespace impl {

// type definitions about timer
using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

// identifier of timer used in 'StartTime' & 'StopTime'
std::size_t timer_id = 0;
// last line number
std::size_t last_line_num;
// last time point
TimePoint last_time_point;
// total time of all timers
std::chrono::microseconds total_time;

// timer guard
// print total elapsed time to stderr when the program exits
// if 'StartTime'/'StopTime' have ever been called
static struct TimerGuard {
  ~TimerGuard() {
    if (!timer_id) return;
    auto us = total_time.count();
    std::cerr << "TOTAL: ";
    PrintTime(us);
    std::cerr << std::endl;
  }

  static void PrintTime(std::uint64_t us) {
    constexpr std::uint64_t kSecond = 1000 * 1000;
    constexpr std::uint64_t kMinute = 60 * kSecond;
    constexpr std::uint64_t kHour = 60 * kMinute;
    std::cerr << us / kHour << "H-";
    us %= kHour;
    std::cerr << us / kMinute << "M-";
    us %= kMinute;
    std::cerr << us / kSecond << "S-" << us % kSecond << "us";
  }
} timer_guard;

bool GetArray(VM &vm, VMOpr &ret, MemId arr) {
  // get length
  std::cin >> ret;
  // get address of array
  auto ptr = vm.mem_pool()->GetAddress(arr);
  if (!ptr) return false;
  // read elements
  for (int i = 0; i < ret; ++i) {
    std::cin >> reinterpret_cast<VMOpr *>(ptr)[i];
  }
  return true;
}

bool PutArray(VM &vm, VMOpr len, MemId arr) {
  // put length
  std::cout << len << ':';
  // get address of array
  auto ptr = vm.mem_pool()->GetAddress(arr);
  if (!ptr) return false;
  // put elements
  for (int i = 0; i < len; ++i) {
    std::cout << ' ' << reinterpret_cast<VMOpr *>(ptr)[i];
  }
  std::cout << std::endl;
  return true;
}

bool StartTime(VMOpr line_num) {
  last_line_num = line_num;
  last_time_point = std::chrono::high_resolution_clock::now();
  return true;
}

bool StopTime(VMOpr line_num) {
  using namespace std::chrono;
  // get elapsed time
  auto cur_time_point = high_resolution_clock::now();
  auto elapsed = cur_time_point - last_time_point;
  auto us = duration_cast<microseconds>(elapsed).count();
  // increase total time
  total_time += duration_cast<microseconds>(elapsed);
  // print timer info to stderr
  std::cerr << "Timer#";
  std::cerr << std::setw(3) << std::setfill('0') << timer_id++ << '@';
  std::cerr << std::setw(4) << std::setfill('0') << last_line_num << '-';
  std::cerr << std::setw(4) << std::setfill('0') << line_num << ": ";
  timer_guard.PrintTime(us);
  std::cerr << std::endl;
  return true;
}

}  // namespace impl

// Eeyore mode wrapper
namespace eeyore {

bool GetInt(VM &vm) {
  VMOpr val;
  std::cin >> val;
  vm.oprs().push(val);
  return true;
}

bool GetCh(VM &vm) {
  vm.oprs().push(std::cin.get());
  return true;
}

bool GetArray(VM &vm) {
  VMOpr ret;
  auto arr = vm.GetParamFromCurPool(0);
  if (!arr) return false;
  if (!impl::GetArray(vm, ret, *arr)) return false;
  vm.oprs().push(ret);
  return true;
}

bool PutInt(VM &vm) {
  auto val = vm.GetParamFromCurPool(0);
  if (!val) return false;
  std::cout << *val;
  return true;
}

bool PutCh(VM &vm) {
  auto val = vm.GetParamFromCurPool(0);
  if (!val) return false;
  std::cout.put(*val);
  return true;
}

bool PutArray(VM &vm) {
  auto len = vm.GetParamFromCurPool(0);
  auto arr = vm.GetParamFromCurPool(1);
  if (!len || !arr) return false;
  return impl::PutArray(vm, *len, *arr);
}

bool StartTime(VM &vm) {
  auto line_num = vm.GetParamFromCurPool(0);
  if (!line_num) return false;
  return impl::StartTime(*line_num);
}

bool StopTime(VM &vm) {
  auto line_num = vm.GetParamFromCurPool(0);
  if (!line_num) return false;
  return impl::StopTime(*line_num);
}

}  // namespace eeyore

// Tigger mode wrapper
namespace tigger {

inline VMOpr &GetRetVal(VM &vm) {
  return vm.regs(static_cast<RegId>(TokenReg::A0));
}

inline VMOpr &GetParam(VM &vm, std::size_t param_id) {
  return vm.regs(static_cast<RegId>(TokenReg::A0) + param_id);
}

void ResetCallerSaveRegs(VM &vm) {
  for (RegId i = static_cast<RegId>(TokenReg::T0);
       i < static_cast<RegId>(TokenReg::A7); ++i) {
    vm.regs(i) = 0xdeadc0de;
  }
}

bool GetInt(VM &vm) {
  ResetCallerSaveRegs(vm);
  std::cin >> GetRetVal(vm);
  return true;
}

bool GetCh(VM &vm) {
  ResetCallerSaveRegs(vm);
  GetRetVal(vm) = std::cin.get();
  return true;
}

bool GetArray(VM &vm) {
  auto arr = GetParam(vm, 0);
  ResetCallerSaveRegs(vm);
  return impl::GetArray(vm, GetRetVal(vm), arr);
}

bool PutInt(VM &vm) {
  std::cout << GetParam(vm, 0);
  ResetCallerSaveRegs(vm);
  return true;
}

bool PutCh(VM &vm) {
  std::cout.put(GetParam(vm, 0));
  ResetCallerSaveRegs(vm);
  return true;
}

bool PutArray(VM &vm) {
  auto len = GetParam(vm, 0), arr = GetParam(vm, 1);
  ResetCallerSaveRegs(vm);
  return impl::PutArray(vm, len, arr);
}

bool StartTime(VM &vm) {
  auto line_num = GetParam(vm, 0);
  ResetCallerSaveRegs(vm);
  return impl::StartTime(line_num);
}

bool StopTime(VM &vm) {
  auto line_num = GetParam(vm, 0);
  ResetCallerSaveRegs(vm);
  return impl::StopTime(line_num);
}

}  // namespace tigger
}  // namespace

void InitEeyoreVM(VM &vm) {
  using namespace eeyore;
  // set memory pool factory function
  vm.set_mem_pool(std::make_unique<SparseMemoryPool>());
  // add library functions
  ADD_LIBS(vm);
  vm.Reset();
}

void InitTiggerVM(VM &vm) {
  using namespace tigger;
  // set memory pool factory function
  vm.set_mem_pool(std::make_unique<DenseMemoryPool>());
  // initialize static registers
  vm.set_static_reg_count(TOKEN_COUNT(TOKEN_REGISTERS));
  vm.set_ret_reg_id(static_cast<RegId>(TokenReg::A0));
  // add library functions
  ADD_LIBS(vm);
  vm.Reset();
  vm.regs(static_cast<RegId>(TokenReg::X0)) = 0;
}
