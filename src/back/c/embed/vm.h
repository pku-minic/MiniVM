#pragma once
#include <stdio.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
struct timeval {
  long tv_sec;
  long tv_usec;
};
static int gettimeofday(struct timeval *tp, struct timezone *tzp) {
  const uint64_t kEpoch = ((uint64_t)116444736000000000ULL);
  SYSTEMTIME system_time;
  FILETIME file_time;
  uint64_t time;
  GetSystemTime(&system_time);
  SystemTimeToFileTime(&system_time, &file_time);
  time = ((uint64_t)file_time.dwLowDateTime);
  time += ((uint64_t)file_time.dwHighDateTime) << 32;
  tp->tv_sec = (long)((time - kEpoch) / 10000000L);
  tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
  return 0;
}
#else
#include <sys/time.h>
#endif

#ifndef CBACKEND
#include "stack.h"
#endif

#ifndef MEM_POOL_SIZE
#define MEM_POOL_SIZE 4096
#endif

static StackFrame *opr_stack;
static uint8_t *mem_pool, *pool_sp;
static size_t timer_id = 0, last_line;
static uint64_t last_time_us, total_time_us = 0;

#ifdef TIGGER_MODE
#ifndef REG_COUNT
#define REG_COUNT 28
#endif
#ifndef ARG_REG_ID
#define ARG_REG_ID 20
#endif
#ifndef RET_REG_ID
#define RET_REG_ID 20
#endif
static int32_t regs[REG_COUNT];
#endif

static void PrintTime(uint64_t us) {
  uint64_t kSecond = 1000 * 1000;
  uint64_t kMinute = 60 * kSecond;
  uint64_t kHour = 60 * kMinute;
  fprintf(stderr, "%dH-%dM-%dS-%dus\n", us / kHour, us / kMinute,
          us / kSecond, us % kSecond);
}

static void InitVM(size_t mem_pool_size) {
  opr_stack = NewStack();
  mem_pool = (uint8_t *)malloc(sizeof(uint8_t) * mem_pool_size);
  pool_sp = mem_pool;
}

static void DestructVM() {
  if (timer_id) {
    fprintf(stderr, "TOTAL: ");
    PrintTime(total_time_us);
  }
  DeleteStack(&opr_stack);
  free(mem_pool);
}

#define APPLY(x) x
#define READ_PARAMS_IMPL(N, ...) APPLY(READ_PARAMS_##N(__VA_ARGS__))
#ifdef TIGGER_MODE
#define READ_PARAM(x) int32_t x = *__cur_arg++;
#define RETURN(x) regs[RET_REG_ID] = (x);
#define READ_PARAMS_1(a) READ_PARAM(a)
#define READ_PARAMS_2(a, b) READ_PARAM(a) READ_PARAM(b)
#define READ_PARAMS_3(a, ...) \
  READ_PARAM(a) APPLY(READ_PARAMS_2(__VA_ARGS__))
#define READ_PARAMS_4(a, ...) \
  READ_PARAM(a) APPLY(READ_PARAMS_3(__VA_ARGS__))
#define READ_PARAMS_5(a, ...) \
  READ_PARAM(a) APPLY(READ_PARAMS_4(__VA_ARGS__))
#define READ_PARAMS(N, ...)               \
  int32_t *__cur_arg = regs + ARG_REG_ID; \
  READ_PARAMS_IMPL(N, __VA_ARGS__)
#else
#define READ_PARAM(x) int32_t x = StackPop(&opr_stack);
#define RETURN(x) StackPush(&opr_stack, x)
#define READ_PARAMS_1(a) READ_PARAM(a)
#define READ_PARAMS_2(a, b) READ_PARAM(b) READ_PARAM(a)
#define READ_PARAMS_3(a, ...) \
  APPLY(READ_PARAMS_2(__VA_ARGS__)) READ_PARAM(a)
#define READ_PARAMS_4(a, ...) \
  APPLY(READ_PARAMS_3(__VA_ARGS__)) READ_PARAM(a)
#define READ_PARAMS_5(a, ...) \
  APPLY(READ_PARAMS_4(__VA_ARGS__)) READ_PARAM(a)
#define READ_PARAMS(N, ...) READ_PARAMS_IMPL(N, __VA_ARGS__)
#endif

static void f_getint() {
  int32_t val;
  scanf("%d", &val);
  RETURN(val);
}
static void f_getch() {
  RETURN(getchar());
}
static void f_getarray() {
  READ_PARAMS(1, arr);
  int32_t len;
  scanf("%d", &len);
  for (int i = 0; i < len; ++i) {
    scanf("%d", mem_pool + arr + i);
  }
  RETURN(len);
}
static void f_putint() {
  READ_PARAMS(1, val);
  printf("%d", val);
}
static void f_putch() {
  READ_PARAMS(1, val);
  printf("%c", val);
}
static void f_putarray() {
  READ_PARAMS(2, len, arr);
  printf("%d:", len);
  for (int i = 0; i < len; ++i) {
    printf(" %d", mem_pool[arr + i]);
  }
  printf("\n");
}
static void f__sysy_starttime() {
  READ_PARAMS(1, line);
  last_line = line;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  last_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
}
static void f__sysy_stoptime() {
  READ_PARAMS(1, line);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t us = (1000000 * tv.tv_sec + tv.tv_usec) - last_time_us;
  total_time_us += us;
  fprintf(stderr, "Timer#03zu@%04zu-%04d: ", timer_id++, last_line, line);
  PrintTime(us);
}
