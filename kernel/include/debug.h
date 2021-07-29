#ifndef __DEBUG_H__
#define __DEBUG_H__

#define  FONT_BLACK          "\033[1;30m"
#define  FONT_RED            "\033[1;31m"
#define  FONT_GREEN          "\033[1;32m"
#define  FONT_YELLOW         "\033[1;33m"
#define  FONT_BLUE           "\033[1;34m"
#define  FONT_PURPLE         "\033[1;35m"
#define  FONT_CYAN           "\033[1;36m"
#define  FONT_WHITE          "\033[1;37m"

#define  FONT_RESET          "\033[0m"

#ifdef  DEBUG 
#define Log(format, ...) \
    printf(FONT_BLUE "[%s,%d,%s] " format FONT_RESET"\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#define print(FONT_COLOR, format, ...) \
    printf(FONT_COLOR format FONT_RESET "\n", \
        ## __VA_ARGS__)
#else
#define Log(format, ...)
#define print(FONT_COLOR, format, ...)
#endif

#ifdef  KMT_DEBUG 
#define KLog(format, ...) \
    printf(FONT_BLUE "[%s,%d,%s] " format FONT_RESET"\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#define Kprint(FONT_COLOR, format, ...) \
    printf(FONT_COLOR format FONT_RESET "\n", \
        ## __VA_ARGS__)
#else
#define KLog(format, ...)
#define Kprint(FONT_COLOR, format, ...)
#endif

#endif