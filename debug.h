#ifndef _DEBUG_H_
#define _DEBUG_H_
  #ifdef DEBUG
    #define DEBUG_WRITE 1
  #else
    #define DEBUG_WRITE 0
  #endif

  #define debug_print(...) \
              do { if (DEBUG_WRITE) fprintf(stderr, __VA_ARGS__); } while (0)
#endif
