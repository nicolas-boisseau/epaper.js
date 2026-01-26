/*****************************************************************************
* | File        :   Debug.h
* | Author      :   Waveshare team
* | Function    :   Debug output
*----------------
* | This version:   V1.0
* | Date        :   2026-01-26
*
******************************************************************************/
#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>

#define USE_DEBUG 1
#if USE_DEBUG
    #define Debug(__info,...) printf("Debug: " __info,##__VA_ARGS__)
#else
    #define Debug(__info,...)
#endif

#endif
