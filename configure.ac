m4_define([pkg_version],  [1.0.17])
m4_define([pkg_revision], [000])
m4_define([pkg_version],  m4_ifdef([pkg_revision], [pkg_version.pkg_revision], [pkg_version]))
m4_define([rpm_revision], m4_ifdef([pkg_revision], ["0.%(date '+%y%m%d')"], [1]))

AC_INIT([e16],[pkg_version],[enlightenment-devel@lists.sourceforge.net])
AM_INIT_AUTOMAKE([foreign dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_MACRO_DIR([m4])

AC_SUBST(E_RPM_REVISION, [rpm_revision])

# reasonable guesses for where stuff is installed
if test "x$prefix" = "xNONE"; then
  prefix=$ac_default_prefix
fi
if test "x$exec_prefix" = "xNONE"; then
  exec_prefix=$prefix
fi

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AM_PROG_CC_C_O

AC_PROG_INSTALL

PKG_PROG_PKG_CONFIG

AC_HEADER_STDC

define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_GCJ_CONFIG], [:])dnl
AC_ENABLE_SHARED
AC_DISABLE_STATIC
LT_INIT([dlopen])

AC_PATH_X
AC_PATH_XTRA

AC_CHECK_HEADERS(alloca.h locale.h stdarg.h wctype.h)

AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
EC_C___ATTRIBUTE__
EC_C___FUNC__
EC_C_VISIBILITY(yes)

AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(wchar_t, 4)

AC_CHECK_FUNCS(snprintf)
AM_CONDITIONAL(USE_ESNPRINTF, test "x$ac_cv_func_snprintf" != "xyes")
AC_CHECK_FUNCS(strcasecmp strcasestr)
AC_CHECK_FUNCS(setenv unsetenv)
AC_CHECK_FUNCS(strdup strndup)

AC_CHECK_FUNCS(blumfrub)
AC_CHECK_FUNCS(buckets_of_erogenous_nym)
AC_CHECK_FUNCS(buttox)

AM_ICONV
AM_LANGINFO_CODESET

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18])

AC_CHECK_FUNCS([clock_gettime], [have_clock_gettime=yes],
  [AC_CHECK_LIB([rt], [clock_gettime], [have_clock_gettime=-lrt],
     [have_clock_gettime=no])])
if test "x$have_clock_gettime" != xno; then
  AC_DEFINE(USE_MONOTONIC_CLOCK, 1, [Use monotonic clock from clock_gettime()])
  if test "x$have_clock_gettime" != xyes; then
    AC_SUBST(CLOCK_LIBS, [$have_clock_gettime])
  fi
fi

AC_ARG_ENABLE(hints-gnome,
  AC_HELP_STRING([--enable-hints-gnome],
                 [compile with GNOME(<2.0) hints support @<:@default=no@:>@]),,
  enable_hints_gnome=no)

if test "x$enable_hints_gnome" = "xyes"; then
  AC_DEFINE(ENABLE_GNOME, 1, [GNOME(<2.0) Hints])
fi
AM_CONDITIONAL(ENABLE_GNOME, test "x$enable_hints_gnome" = "xyes")

AC_ARG_ENABLE(sound,
  AC_HELP_STRING([--enable-sound],
                 [compile with sound support (pulseaudio/esound/no)@<:@default=pulseaudio@:>@]),,
  enable_sound=pulseaudio)
case x$enable_sound in
xyes)
  enable_sound=pulseaudio;;
xpulseaudio|xesound)
  ;;
*)
  enable_sound=no;;
esac

AC_ARG_WITH(sndldr,
  AC_HELP_STRING([--with-sndldr],
                 [select sound loader (audiofile/sndfile/none) @<:@default=sndfile@:>@]),,
  with_sndldr=sndfile)

if test "x$with_sndldr" = "xsndfile"; then
  PKG_CHECK_MODULES(SNDFILE, sndfile,
    AC_DEFINE(USE_SOUND_LOADER_SNDFILE, 1, [Use sndfile sound loader]),
    with_sndldr=none)
fi
if test "x$with_sndldr" = "xaudiofile"; then
  PKG_CHECK_MODULES(AUDIOFILE, audiofile,
    AC_DEFINE(USE_SOUND_LOADER_AUDIOFILE, 1, [Use audiofile sound loader]),
    with_sndldr=none)
fi

if test "x$enable_sound" = "xpulseaudio"; then
  PKG_CHECK_MODULES(PA, libpulse,
    AC_DEFINE(HAVE_SOUND, 1, [Sound support])
    AC_DEFINE(HAVE_SOUND_PA, 1, [PulseAudio sound support]),
    enable_sound=no)
fi
AM_CONDITIONAL(USE_LIBPA, test "x$enable_sound" = "xpulseaudio")

if test "x$enable_sound" = "xesound"; then
  AM_PATH_ESD(0.2.17,,[
    enable_sound=no
    AC_MSG_WARN([EsounD sound support was requested but not found.])
  ])
  if test "x$enable_sound" = "xesound"; then
    AC_DEFINE(HAVE_SOUND, 1, [Sound support])
    AC_DEFINE(HAVE_SOUND_ESD, 1, [EsounD sound support])
  fi
fi
AM_CONDITIONAL(USE_LIBESD, test "x$enable_sound" = "xesound")

# Save CPPFLAGS/LDFLAGS and add X_... to each
SAVE_CPPFLAGS="$CPPFLAGS"
SAVE_LDFLAGS="$LDFLAGS"
CPPFLAGS="$X_CFLAGS $CPPFLAGS"
LDFLAGS="$X_LIBS $LDFLAGS"

PKG_CHECK_MODULES(IMLIB2, imlib2 >= 1.2.0, have_good_imlib2=yes)
AC_CHECK_LIB(Imlib2, imlib_context_set_mask_alpha_threshold,
  AC_DEFINE(HAVE_IMLIB_CONTEXT_SET_MASK_ALPHA_THRESHOLD, 1,
            [Imlib2 has imlib_context_set_mask_alpha_threshold]),,
  $IMLIB2_LIBS)
AC_CHECK_LIB(Imlib2, imlib_context_disconnect_display,
  AC_DEFINE(HAVE_IMLIB_CONTEXT_DISCONNECT_DISPLAY, 1,
            [Imlib2 has imlib_context_disconnect_display]),,
  $IMLIB2_LIBS)

AC_ARG_ENABLE(sm,
  AC_HELP_STRING([--enable-sm], [compile with session management support @<:@default=yes@:>@]),,
  enable_sm=yes)
if test "x$enable_sm" = "xyes"; then
  AC_CHECK_HEADERS(X11/SM/SMlib.h,, enable_sm=no)
  AC_CHECK_LIB(SM, SmcOpenConnection, SM_LIBS="-lSM -lICE", enable_sm=no)
  AC_SUBST(SM_LIBS)
fi
if test "x$enable_sm" = "xyes"; then
  AC_DEFINE(USE_SM, 1, [Session management support])
fi

AC_ARG_ENABLE(pango,
  AC_HELP_STRING([--enable-pango], [compile with pango-xft support @<:@default=yes@:>@]),,
  enable_pango=yes)
if test "x$enable_pango" = "xyes"; then
  PKG_CHECK_MODULES(PANGO, pangoxft, AC_DEFINE(USE_PANGO, 1, [pango-xft support]), enable_pango=no)
fi
AM_CONDITIONAL(USE_LIBPANGO, test "x$enable_pango" = "xyes")

AC_ARG_ENABLE(xft,
  AC_HELP_STRING([--enable-xft], [compile with Xft support @<:@default=yes@:>@]),,
  enable_xft=yes)
if test "x$enable_xft" = "xyes"; then
  PKG_CHECK_MODULES(XFT, xft, AC_DEFINE(USE_XFT, 1, [Xft support]), enable_xft=no)
fi
AM_CONDITIONAL(USE_LIBXFT, test "x$enable_xft" = "xyes")

AC_ARG_ENABLE(xi2,
  AC_HELP_STRING([--enable-xi2], [compile with XI2 support (experimental) @<:@default=no@:>@]),,
  enable_xi2=no)
if test "x$enable_xi2" = "xyes"; then
  PKG_CHECK_MODULES(XI, xi >= 1.3, AC_DEFINE(USE_XI2, 1, [XI2 support]), enable_xi2=no)
fi

# Is this reasonably correct?
AC_ARG_ENABLE(glx,
  AC_HELP_STRING([--enable-glx], [compile with GLX support (experimental) @<:@default=no@:>@]),,
  enable_glx=no)
if test "x$enable_glx" = "xyes"; then
  AC_CHECK_HEADERS(GL/gl.h GL/glu.h GL/glx.h, , AC_MSG_ERROR([Cannot find GLX headers]))
  AC_CHECK_LIB(GL, glXCreateNewContext,
    [
      AC_DEFINE(USE_GLX, 1, [GLX support])
      GLX_LIBS="-lGL -lGLU"
    ],[
      AC_MSG_ERROR([Cannot find GLX libraries])
    ])
  AC_CHECK_LIB(GL, glXBindTexImageEXT,
      AC_DEFINE(HAVE_GLX_glXBindTexImageEXT, 1, [glXBindTexImageEXT is available]),
      GLX_LIBS="$GLX_LIBS -ldl")
  AC_SUBST(GLX_LIBS)
fi
AM_CONDITIONAL(ENABLE_GLX, test "x$enable_glx" = "xyes")

AC_CHECK_LIB(Xext, XShapeQueryExtension,
  E_X_LIBS="-lXext $E_X_LIBS",
  AC_MSG_ERROR([Shape support is required but was not found]),
  -lX11)

AC_ARG_ENABLE(xinerama,
  AC_HELP_STRING([--enable-xinerama], [compile with xinerama support @<:@default=yes@:>@]),,
  enable_xinerama=yes)
if test "x$enable_xinerama" = "xyes"; then
  AC_CHECK_LIB(Xinerama, XineramaQueryExtension,
    AC_DEFINE(USE_XINERAMA, 1, [Xinerama support])
    XINERAMA_LIBS="-lXinerama",
    enable_xinerama=no
    AC_MSG_WARN([Xinerama support was requested but not found]),
    -lXext -lX11)
fi
AC_SUBST(XINERAMA_LIBS)

AC_ARG_ENABLE(xsync,
  AC_HELP_STRING([--enable-xsync], [compile with SYNC support @<:@default=yes@:>@]),,
  enable_xsync=yes)
if test "x$enable_xsync" = "xyes"; then
  AC_CHECK_LIB(Xext, XSyncQueryExtension,
    AC_DEFINE(USE_XSYNC, 1, [SYNC support]),
    enable_xsync=no
    AC_MSG_WARN([Sync support was requested but not found]),
    -lX11)
fi

AC_ARG_ENABLE(xscrnsaver,
  AC_HELP_STRING([--enable-xscrnsaver],
                 [compile with ScreenSaver support (experimental/not useful) @<:@default=no@:>@]),,
  enable_xscrnsaver=no)
if test "x$enable_xscrnsaver" = "xyes"; then
  AC_CHECK_LIB(Xss, XScreenSaverQueryExtension,
    AC_DEFINE(USE_XSCREENSAVER, 1, [ScreenSaver support])
    E_X_LIBS="-lXss $E_X_LIBS",
    enable_xscrnsaver=no
    AC_MSG_WARN([ScreenSaver support was requested but not found]),
    -lXext -lX11)
fi

AC_ARG_ENABLE(xrandr,
  AC_HELP_STRING([--enable-xrandr], [compile with RandR support @<:@default=yes@:>@]),,
  enable_xrandr=yes)
if test "x$enable_xrandr" = "xyes"; then
  PKG_CHECK_MODULES(XRANDR, xrandr,
    AC_DEFINE(USE_XRANDR, 1, [RandR support])
    PKG_CHECK_EXISTS(xrandr >= 1.2, AC_DEFINE(HAVE_XRANDR_12, 1, [RandR 1.2 support])
      have_randr_12=y)
    PKG_CHECK_EXISTS(xrandr >= 1.3, AC_DEFINE(HAVE_XRANDR_13, 1, [RandR 1.3 support])
      have_randr_13=y),
    enable_xrandr=no
    AC_MSG_WARN([RandR support was requested but not found]) )
fi

AC_ARG_ENABLE(xrender,
  AC_HELP_STRING([--enable-xrender], [compile with Render support @<:@default=yes@:>@]),,
  enable_xrender=yes)
if test "x$enable_xrender" = "xyes"; then
  AC_CHECK_LIB(Xrender, XRenderQueryExtension,
    AC_DEFINE(USE_XRENDER, 1, [Render support])
    E_X_LIBS="-lXrender $E_X_LIBS",
    enable_xrender=no
    AC_MSG_WARN([Render support was requested but not found]),
    -lX11)
  AC_CHECK_HEADERS(X11/extensions/Xrender.h,,,[#include <X11/Xlib.h>])
fi

# FIXME - Do this properly
AC_ARG_ENABLE(composite,
  AC_HELP_STRING([--enable-composite], [compile with Composite support @<:@default=yes@:>@]),,
  enable_composite=yes)
if test "x$enable_xrender" != "xyes"; then enable_composite=no; fi
if test "x$enable_composite" = "xyes"; then
  AC_CHECK_LIB(Xcomposite, XCompositeQueryExtension,
    AC_DEFINE(USE_COMPOSITE, 1, [Composite support])
    E_X_LIBS="-lXcomposite -lXdamage -lXfixes $E_X_LIBS",
    enable_composite=no,
    -lXext -lX11)
  AC_CHECK_HEADERS(X11/extensions/Xcomposite.h ,, enable_composite=no, [#include <X11/Xlib.h>])
  AC_CHECK_HEADERS(X11/extensions/Xdamage.h    ,, enable_composite=no, [#include <X11/Xlib.h>])
  AC_CHECK_HEADERS(X11/extensions/Xfixes.h     ,, enable_composite=no, [#include <X11/Xlib.h>])
  if test "x$enable_composite" != "xyes"; then
    AC_MSG_WARN([Composite support was requested but required component was not found])
  fi
fi
AM_CONDITIONAL(ENABLE_COMPOSITE, test "x$enable_composite" = "xyes")

AC_ARG_ENABLE(xpresent,
  AC_HELP_STRING([--enable-xpresent], [compile with Present support (experimental) @<:@default=no@:>@]),,
  enable_xpresent=no)
if test "x$enable_xpresent" = "xyes"; then
  PKG_CHECK_MODULES(XPRESENT, xpresent,
    AC_DEFINE(USE_XPRESENT, 1, [Present support]),
    enable_xpresent=no
    AC_MSG_WARN([Present support was requested but not found]) )
fi

AC_ARG_ENABLE(zoom,
  AC_HELP_STRING([--enable-zoom], [compile with zoom support @<:@default=yes@:>@]),,
  enable_zoom=yes)
if test "x$enable_zoom" = "xyes"; then
  if test "x$have_randr_13" = "xy"; then
    enable_zoom=xrandr
  else
    enable_zoom=xf86vm
  fi
fi
if test "x$enable_zoom" = "xxf86vm"; then
  AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryExtension,
    AC_DEFINE(ENABLE_ZOOM, 1, [Zoom Support])
    AC_DEFINE(USE_ZOOM_XF86VM, 1, [Use xf86vm for zooming (deprecated)])
    E_X_LIBS="-lXxf86vm $E_X_LIBS",
    enable_zoom=no
    AC_MSG_WARN([Zoom support was requested but not found]),
    -lXext -lX11)
fi
if test "x$enable_zoom" = "xxrandr"; then
  if test "x$have_randr_13" = "xy"; then
    AC_DEFINE(ENABLE_ZOOM, 1, [Zoom Support])
  else
    enable_zoom=no
    AC_MSG_WARN([Zoom support was requested but not found])
  fi
fi
AM_CONDITIONAL(ENABLE_ZOOM, test "x$enable_zoom" != "xno")

AC_ARG_ENABLE(dbus,
  AC_HELP_STRING([--enable-dbus], [compile with D-Bus support (experimental) @<:@default=no@:>@]),,
  enable_dbus=no)
if test "x$enable_dbus" = "xyes"; then
  PKG_CHECK_MODULES(DBUS, dbus-1, AC_DEFINE(USE_DBUS, 1, [dbus support]), enable_dbus=no)
fi
AM_CONDITIONAL(ENABLE_DBUS, test "x$enable_dbus" = "xyes")

AC_CHECK_LIB(Fridge,mass_quantities_of_bass_ale)
AC_CHECK_LIB(Fridge,mass_quantities_of_any_ale,, [
  echo "Warning: No ales were found in your refrigerator."
  echo "  We highly suggest that you rectify this situation immediately."
])

# GNOME session support
AC_ARG_WITH(gnome,
  AC_HELP_STRING([--with-gnome], [compile with gnome support @<:@default=auto@:>@]),,
  with_gnome=auto)
if test "x$with_gnome" = "xauto"; then
  with_gnome2=yes
  with_gnome3=yes
fi
if test "x$with_gnome" = "xgnome2"; then
  with_gnome2=yes
fi
if test "x$with_gnome" = "xgnome3"; then
  with_gnome3=yes
fi
AM_CONDITIONAL(WITH_GNOME2, test "x$with_gnome2" = "xyes")
AM_CONDITIONAL(WITH_GNOME3, test "x$with_gnome3" = "xyes")

# Restore CPPFLAGS/LDFLAGS
CPPFLAGS="$SAVE_CPPFLAGS"
LDFLAGS="$SAVE_LDFLAGS"

AC_ARG_ENABLE(libhack,
  AC_HELP_STRING([--enable-libhack], [build window mode helper library @<:@default=yes@:>@]),,
  enable_libhack=yes)
if test "x$enable_libhack" = "xyes"; then
  AC_CHECK_LIB(dl, dlopen, DLOPEN_LIBS=-ldl, enable_libhack=no)
  AC_SUBST(DLOPEN_LIBS)
fi
if test "x$enable_libhack" = "xyes"; then
  AC_DEFINE(USE_LIBHACK, 1, [Use window mode helper library])
fi
AM_CONDITIONAL(BUILD_LIBHACK, test "x$enable_libhack" = "xyes")

AC_ARG_ENABLE(libtrip,
  AC_HELP_STRING([--enable-libtrip], [build debug library @<:@default=no@:>@]),,
  enable_libtrip=no)
AC_CHECK_HEADERS(execinfo.h,, enable_libtrip=no)
AC_CHECK_FUNCS(backtrace backtrace_symbols,, enable_libtrip=no)
AM_CONDITIONAL(BUILD_LIBTRIP, test "x$enable_libtrip" = "xyes")

AC_ARG_ENABLE(modules,
  AC_HELP_STRING([--enable-modules], [enable support for loadable modules @<:@default=no@:>@]),,
  enable_modules=no)
if test "x$enable_modules" = "xyes"; then
  AC_CHECK_LIB(dl, dlopen, DLOPEN_LIBS=-ldl, enable_modules=no)
  AC_SUBST(DLOPEN_LIBS)
fi
if test "x$enable_modules" = "xyes"; then
  AC_DEFINE(USE_MODULES, 1, [Loadable module support])
fi
AM_CONDITIONAL(BUILD_MODULES, test "x$enable_modules" = "xyes")

AC_ARG_ENABLE(dialogs,
  AC_HELP_STRING([--enable-dialogs],
                 [enable support for configuration dialogs @<:@default=yes@:>@]),,
  enable_dialogs=yes)
if test "x$enable_dialogs" = "xyes"; then
  AC_DEFINE(ENABLE_DIALOGS, 1, [Configuration dialog support])
fi

AC_ARG_ENABLE(pseudotrans,
  AC_HELP_STRING([--enable-pseudotrans],
                 [enable support for pseudo-transparency @<:@default=no@:>@]),,
  enable_pseudotrans=no)
if test "x$enable_pseudotrans" = "xyes"; then
  AC_DEFINE(ENABLE_TRANSPARENCY,       1, [Enable pseudotransparency])
  AC_DEFINE(ENABLE_THEME_TRANSPARENCY, 1, [Enable adjusting theme element pseudotransparency])
fi

AC_ARG_ENABLE(container,
  AC_HELP_STRING([--enable-container],
                 [use container window (experimental) @<:@default=yes@:>@]),,
  enable_container=yes)
if test "x$enable_container" = "xyes"; then
  AC_DEFINE(USE_CONTAINER_WIN, 1, [Use container window])
fi

AM_MISSING_PROG(XMLTO, xmlto)
AC_ARG_ENABLE(mans,
  AC_HELP_STRING([--enable-mans], [install man page @<:@default=yes@:>@]),,
  enable_mans=yes)
AM_CONDITIONAL([INSTALL_MANS], [test "x$enable_mans" = "xyes"])
AC_ARG_ENABLE(docs,
  AC_HELP_STRING([--enable-docs], [install html docs @<:@default=yes@:>@]),,
  enable_docs=yes)
AM_CONDITIONAL([INSTALL_DOCS], [test "x$enable_docs" = "xyes"])

AC_ARG_ENABLE(gcc-cpp,
  AC_HELP_STRING([--enable-gcc-cpp], [compile everything as if it were C++ @<:@default=no@:>@]),,
  enable_gcc_cpp=no)
if test "x$enable_gcc_cpp" = "xyes"; then
  CC="g++"
  CPPFLAGS="$CPPFLAGS -x c++"
  EC_C_WARNINGS([cpp])
else
  EC_C_WARNINGS()
fi

AC_DEFINE(USE_EXT_INIT_WIN,          1, [Use external init window during restart])

AC_DEFINE(ENABLE_DESKRAY,            0, [Enable deskrays (not functional)])

AC_SUBST(E_X_LIBS)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
Makefile
src/Makefile
dox/Makefile
dox/E-docs/Makefile
eesh/Makefile
epp/Makefile
lib/Makefile
config/Makefile
icons/Makefile
pix/Makefile
themes/Makefile
fonts/Makefile
scripts/Makefile
misc/Makefile
docs/Makefile
po/Makefile.in
e16.spec
])
AC_OUTPUT

if test "x$have_good_imlib2" != "xyes"; then
  echo "***************************************************************"
  echo "*** WARNING: imlib2 >= 1.2.0 was not detected               ***"
  echo "*** WARNING: If you indeed do not have this, expect trouble ***"
  echo "***************************************************************"
fi

echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE Version $VERSION"
echo "------------------------------------------------------------------------"
echo
echo "Configuration summary"
echo
echo "Support for"
echo "  Localisation ................. $USE_NLS"
echo "  Sound ........................ $enable_sound"
echo "  Sound loader ................. $with_sndldr"
echo "  Old GNOME hints .............. $enable_hints_gnome"
echo "  Session management ........... $enable_sm"
echo "  Zoom ......................... $enable_zoom"
echo "  Pango ........................ $enable_pango"
echo "  Xft .......................... $enable_xft"
echo "  Xinerama ..................... $enable_xinerama"
echo "  RandR ........................ $enable_xrandr"
echo "  Render ....................... $enable_xrender"
echo "  Sync ......................... $enable_xsync"
echo "  Composite .................... $enable_composite"
echo "  GNOME session support ........ $with_gnome"
echo "  Modules ...................... $enable_modules"
echo "  Visibility hiding ............ $enable_visibility_hiding (only useful with modules)"
echo "  Window mode helper library ... $enable_libhack"
echo "  Dialogs ...................... $enable_dialogs"
echo "  Pseudo-transparency .......... $enable_pseudotrans"
echo
echo "Experimental options - DO NOT USE unless you know what you are doing"
echo "  GLX .......................... $enable_glx"
echo "  ScreenSaver .................. $enable_xscrnsaver"
echo "  D-Bus ........................ $enable_dbus"
echo "  XI2 .......................... $enable_xi2"
echo "  Present....................... $enable_xpresent"
echo "  Use container window ......... $enable_container"
echo
echo "Installation path .............. $prefix"
echo "  Install HTML docs ............ $enable_docs"
echo "  Install man page ............. $enable_mans"
echo
echo "Compilation .................... make"
echo "Installation ................... make install"
echo
