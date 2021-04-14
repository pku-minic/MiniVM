#pragma once
#include <stdlib.h>
#include <stddef.h>

#ifndef CBACKEND
#include "define.h"
#endif

const size_t kMaxFrameSize = 512;

typedef struct StackFrame {
  vmopr_t data[kMaxFrameSize];
  size_t sp;
  struct StackFrame *last;
} StackFrame;

static StackFrame *NewStack() {
  StackFrame *frame = (StackFrame *)malloc(sizeof(StackFrame));
  frame->sp = 0;
  frame->last = NULL;
  return frame;
}

static void DeleteStack(StackFrame **sref) {
  StackFrame *cur = *sref;
  while (cur) {
    StackFrame *next = cur->last;
    free(cur);
    cur = next;
  }
}

static void StackPush(StackFrame **sref, vmopr_t val) {
  StackFrame *s = *sref;
  s->data[s->sp++] = val;
  if (s->sp >= kMaxFrameSize) {
    StackFrame *next = NewStack();
    next->last = s;
    *sref = next;
  }
}

static vmopr_t StackPop(StackFrame **sref) {
  StackFrame *s = *sref;
  if (!s->sp) {
    *sref = s->last;
    free(s);
    s = *sref;
  }
  return s->data[--s->sp];
}
