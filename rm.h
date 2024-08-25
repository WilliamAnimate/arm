#define DEBUG

#ifndef RM_DOT_H
#define RM_DOT_H
#endif

#ifdef DEBUG
    // #define DEBUGPRINT(_fmt, ...)  fprintf(stderr, "[file %s, line %d]: " _fmt, __FILE__, __LINE__, __VA_ARGS__)
    #define DEBUGPRINT(fmt, ...) fprintf(stderr, "[file %s, line %d]: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUGPRINT(_fmt, ...)  // skip
#endif
