#ifndef LIBP101_IO_P101_STDIO_H
#define LIBP101_IO_P101_STDIO_H

/*
 * Copyright 2026 D'Arcy Smith.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBP101_IO_SHARED_DECLARATIONS
    #define LIBP101_IO_SHARED_DECLARATIONS
    #include <aio.h>
    #include <p101_env/env.h>
    #include <p101_error/attributes.h>
    #include <poll.h>
    #include <stddef.h>
    #include <stdio.h>
    #include <sys/select.h>
    #include <sys/types.h>
    #include <sys/uio.h>
    #include <unistd.h>
#endif    // LIBP101_IO_SHARED_DECLARATIONS

#ifdef __cplusplus
extern "C"
{
#endif

    FILE   *p101_fdopen(const struct p101_env *env, struct p101_error *err, int fildes, const char *mode) P101_ATTR_WARN_UNUSED_RESULT;
    int     p101_fileno(const struct p101_env *env, struct p101_error *err, FILE *stream);
    void    p101_flockfile(const struct p101_env *env, FILE *file);
    FILE   *p101_fmemopen(const struct p101_env *env, struct p101_error *err, void *restrict buf, size_t size, const char *restrict mode) P101_ATTR_WARN_UNUSED_RESULT;
    int     p101_fpurge(const struct p101_env *env, struct p101_error *err, FILE *stream);
    int     p101_fseeko(const struct p101_env *env, struct p101_error *err, FILE *stream, off_t offset, int whence);
    off_t   p101_ftello(const struct p101_env *env, struct p101_error *err, FILE *stream);
    int     p101_ftrylockfile(const struct p101_env *env, FILE *file);
    void    p101_funlockfile(const struct p101_env *env, FILE *file);
    int     p101_getc_unlocked(const struct p101_env *env, struct p101_error *err, FILE *stream);
    int     p101_getchar_unlocked(const struct p101_env *env, struct p101_error *err);
    ssize_t p101_getdelim(const struct p101_env *env, struct p101_error *err, char **restrict lineptr, size_t *restrict n, int delimiter, FILE *restrict stream);
    ssize_t p101_getline(const struct p101_env *env, struct p101_error *err, char **restrict lineptr, size_t *restrict n, FILE *restrict stream);
    FILE   *p101_open_memstream(const struct p101_env *env, struct p101_error *err, char **bufp, size_t *sizep) P101_ATTR_WARN_UNUSED_RESULT;
    int     p101_putc_unlocked(const struct p101_env *env, struct p101_error *err, int c, FILE *stream);
    int     p101_putchar_unlocked(const struct p101_env *env, struct p101_error *err, int c);
    int     p101_setbuffer(const struct p101_env *env, struct p101_error *err, FILE *stream, char *buf, size_t size);
    void    p101_setlinebuf(const struct p101_env *env, FILE *stream);
    int     p101_vdprintf(const struct p101_env *env, struct p101_error *err, int fildes, const char *restrict format, va_list ap) P101_ATTR_PRINTF(4, 0);

#ifdef __cplusplus
}
#endif

#endif    // LIBP101_IO_P101_STDIO_H
