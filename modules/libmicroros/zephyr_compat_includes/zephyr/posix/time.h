/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compat shim: rcutils/src/time_unix.c (jazzy) includes <zephyr/posix/time.h>
 * which existed in Zephyr 3.1–4.1 but was renamed to <zephyr/posix/posix_time.h>
 * in Zephyr 4.2.
 *
 * Forwarding to the new <zephyr/posix/posix_time.h> caused type collisions
 * with the SDK's own <time.h> (time_t, struct itimerspec, locale_t, etc.) on
 * SDK 1.0.0-beta1 + picolibc — picolibc already provides the full time API
 * via the toolchain headers.
 *
 * Make this a no-op: rcutils' time_unix.c also pulls in <time.h> via other
 * means, and that's enough on this configuration.
 */
#ifndef ZEPHYR_POSIX_TIME_COMPAT_H
#define ZEPHYR_POSIX_TIME_COMPAT_H
#endif
