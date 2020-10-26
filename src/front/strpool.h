#ifndef MINIVM_FRONT_STRPOOL_H_
#define MINIVM_FRONT_STRPOOL_H_

// NOTE: thread unsafe!

// allocate memory for 'str', copy it's value into it
// returns the pointer to that memory
const char *NewStr(const char *str);

// free all allocated strings
void FreeAllStrs();

#endif  // MINIVM_FRONT_STRPOOL_H_
