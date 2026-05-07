/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SDK / libc compatibility shim for the libmicroros colcon meta-build when
 * built against Zephyr SDK 1.0.0-beta1 + picolibc + Zephyr >= 4.2.
 *
 * Force-included via -include from libmicroros's external project CFLAGS
 * (see CMakeLists.txt in this directory).
 *
 * Two issues this papers over:
 *
 * 1. The SDK 1.0.0-beta1 newlib's <stdlib.h> has a header-guard ordering bug
 *    in its `#if __STDC_WANT_LIB_EXT1__ == 1` block (C11 Annex K bounds-
 *    checking interfaces). When that macro is set to 1, stdlib.h includes
 *    <sys/_types.h> expecting the typedefs for __errno_t / __rsize_t to
 *    appear, but if <sys/_types.h> was already included earlier without the
 *    macro set, the header guard prevents reprocessing and the typedefs
 *    never come into scope. Forcing the macro to 0 sidesteps the whole
 *    Annex K block — fine because no micro-ROS code uses Annex K.
 *
 * 2. rcutils/src/logging.c calls isatty() to decide whether to colorize log
 *    output. With Zephyr's POSIX layer + picolibc + this SDK's newlib, the
 *    declaration isn't visible. We forward-declare it here; the symbol is
 *    provided by a stub in samples/.../shims.c (color codes on the HTIF
 *    console add noise — "not a tty" is the right semantic anyway).
 */
#ifndef LIBMICROROS_ZEPHYR_SDK_COMPAT_H
#define LIBMICROROS_ZEPHYR_SDK_COMPAT_H

#undef  __STDC_WANT_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__ 0

/* rcutils/src/strcasecmp.c calls strcasecmp/strncasecmp from <strings.h>. The
 * SDK's <string.h> only forwards-includes <strings.h> when __BSD_VISIBLE is
 * set, which it isn't under _POSIX_C_SOURCE=200809L. Force-include here so
 * the declarations are visible everywhere libmicroros builds. */
#include <strings.h>

#ifdef __cplusplus
extern "C" {
#endif

int isatty(int fd);

#ifdef __cplusplus
}
#endif

#endif /* LIBMICROROS_ZEPHYR_SDK_COMPAT_H */
