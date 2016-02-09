dnl Copyright (C) 2004-2008 Kim Woelders
dnl This code is public domain and can be freely used or copied.
dnl Originally snatched from somewhere...

dnl Macro for checking if the compiler supports __attribute__

dnl Usage: EC_C___ATTRIBUTE__

dnl Call AC_DEFINE for HAVE___ATTRIBUTE__ and __UNUSED__.
dnl If the compiler supports __attribute__, HAVE___ATTRIBUTE__ is
dnl defined to 1 and __UNUSED__ is defined to __attribute__((unused))
dnl otherwise, HAVE___ATTRIBUTE__ is not defined and __UNUSED__ is
dnl defined to nothing.

AC_DEFUN([EC_C___ATTRIBUTE__],
[
  AC_MSG_CHECKING(for __attribute__)
  AC_CACHE_VAL(ec_cv___attribute__, [
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM(
      [[
#include <stdlib.h>
      ]], [[
int foo(int x __attribute__ ((unused))) { exit(1); }
      ]])
    ],
    ec_cv___attribute__=yes,
    ec_cv___attribute__=no)
  ])

  if test "$ec_cv___attribute__" = "yes"; then
    AC_DEFINE(HAVE___ATTRIBUTE__, 1, [Define to 1 if your compiler has __attribute__])
    AC_DEFINE(__UNUSED__, __attribute__((unused)), [Macro declaring a function argument to be unused])
  else
    AC_DEFINE(__UNUSED__, , [Macro declaring a function argument to be unused])
  fi
  AC_MSG_RESULT($ec_cv___attribute__)
])
