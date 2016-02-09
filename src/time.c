/*
 * Copyright (C) 2011-2015 Kim Woelders
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "config.h"

#if USE_MONOTONIC_CLOCK
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "util.h"

unsigned int
GetTimeMs(void)
{
#if USE_MONOTONIC_CLOCK
   struct timespec     ts;

   clock_gettime(CLOCK_MONOTONIC, &ts);

   return (unsigned int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
   struct timeval      timev;

   gettimeofday(&timev, NULL);

   return (unsigned int)(timev.tv_sec * 1000 + timev.tv_usec / 1000);
#endif
}

unsigned int
GetTimeUs(void)
{
#if USE_MONOTONIC_CLOCK
   struct timespec     ts;

   clock_gettime(CLOCK_MONOTONIC, &ts);

   return (unsigned int)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#else
   struct timeval      timev;

   gettimeofday(&timev, NULL);

   return (unsigned int)(timev.tv_sec * 1000000 + timev.tv_usec);
#endif
}

void
SleepUs(unsigned int tus)
{
   struct timespec     ts;

   ts.tv_sec = tus / 1000000;
   tus -= ts.tv_sec * 1000000;
   ts.tv_nsec = tus * 1000;

   while (nanosleep(&ts, &ts))
      ;
}
