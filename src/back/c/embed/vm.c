#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifndef STACK_SIZE
#define STACK_SIZE 4096
#endif
#ifndef MEM_POOL_SIZE
#define MEM_POOL_SIZE 1048576
#endif

typedef int32_t vmopr_t;
typedef uint32_t vmaddr_t;

static vmopr_t *opr_stack;
static uint8_t *mem_pool;
static vmaddr_t pool_sp = 0;
static size_t stack_sp = 0, timer_id = 0, last_line;
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
static vmopr_t regs[REG_COUNT];
#endif

static void PrintTime(uint64_t us) {
  const uint64_t kSecond = 1000 * 1000;
  const uint64_t kMinute = 60 * kSecond;
  const uint64_t kHour = 60 * kMinute;
  uint64_t h = us / kHour;
  us %= kHour;
  uint64_t m = us / kMinute;
  us %= kMinute;
  fprintf(stderr, "%" PRIu64 "H-%" PRIu64 "M-%" PRIu64 "S-%" PRIu64 "us\n",
          h, m, us / kSecond, us % kSecond);
}

static void InitVM(size_t stack_size, size_t mem_pool_size) {
  opr_stack = (vmopr_t *)malloc(sizeof(vmopr_t) * stack_size);
  mem_pool = (uint8_t *)malloc(sizeof(uint8_t) * mem_pool_size);
}

static void DestructVM() {
  if (timer_id) {
    fprintf(stderr, "TOTAL: ");
    PrintTime(total_time_us);
  }
  free(opr_stack);
  free(mem_pool);
}

static void VMEntry();

#define INLINE static inline __attribute__((always_inline))
INLINE void PushValue(vmopr_t val) { opr_stack[stack_sp++] = val; }
INLINE void PokeValue(vmopr_t val) { opr_stack[stack_sp - 1] = val; }
INLINE vmopr_t PopValue() { return opr_stack[--stack_sp]; }
INLINE vmopr_t PeekValue() { return opr_stack[stack_sp - 1]; }
INLINE void Clear() { stack_sp = 0; }
INLINE size_t StackSize() { return stack_sp; }
INLINE void Break() { raise(SIGTRAP); }

#define APPLY(x) x
#define READ_PARAMS_IMPL(N, ...) APPLY(READ_PARAMS_##N(__VA_ARGS__))
#ifdef TIGGER_MODE
#define READ_PARAM(x) vmopr_t x = *__cur_arg++;
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
  vmopr_t *__cur_arg = regs + ARG_REG_ID; \
  READ_PARAMS_IMPL(N, __VA_ARGS__)
#else
#define READ_PARAM(x) vmopr_t x = PopValue();
#define RETURN(x) PushValue(x)
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
  vmopr_t val;
  scanf("%" SCNd32, &val);
  RETURN(val);
}
static void f_getch() {
  RETURN(getchar());
}
static void f_getarray() {
  READ_PARAMS(1, arr);
  vmopr_t len;
  scanf("%" SCNd32, &len);
  for (int i = 0; i < len; ++i) {
    scanf("%" SCNd32, (vmopr_t *)(mem_pool + arr + i * 4));
  }
  RETURN(len);
}
static void f_putint() {
  READ_PARAMS(1, val);
  printf("%" PRId32, val);
}
static void f_putch() {
  READ_PARAMS(1, val);
  putchar(val);
}
static void f_putarray() {
  READ_PARAMS(2, len, arr);
  printf("%" PRId32 ":", len);
  for (int i = 0; i < len; ++i) {
    printf(" %" PRId32, *(vmopr_t *)(mem_pool + arr + i * 4));
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
  fprintf(stderr, "Timer#%03zu@%04zu-%04" PRId32 ": ", timer_id++,
          last_line, line);
  PrintTime(us);
}

int main(int argc, const char *argv[]) {
  size_t stack_size = STACK_SIZE, mem_pool_size = MEM_POOL_SIZE;
  if (argc > 1) {
    for (int i = 1; i < argc; ++i) {
      if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
        printf(
            "This is a program generated by the MiniVM C backend.\n"
            "usage: %s [-h | --help]"
            " [-s SIZE | --stack-size SIZE]"
            " [-m SIZE | --mem-pool-size SIZE]\n",
            argv[0]);
        return 0;
      }
      else if ((!strcmp(argv[i], "-s") ||
                !strcmp(argv[i], "--stack-size")) &&
               i + 1 < argc) {
        stack_size = atoi(argv[++i]);
      }
      else if ((!strcmp(argv[i], "-m") ||
                !strcmp(argv[i], "--mem-pool-size")) &&
               i + 1 < argc) {
        mem_pool_size = atoi(argv[++i]);
      }
      else {
        printf("Invalid argument, try '%s -h'.\n", argv[0]);
        return 1;
      }
    }
  }
  InitVM(stack_size, mem_pool_size);
  VMEntry();
  vmopr_t ret;
#ifdef TIGGER_MODE
  ret = regs[RET_REG_ID];
#else
  ret = PopValue();
#endif
  DestructVM();
  return ret;
}
