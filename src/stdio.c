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

#include "p101_io/p101_stdio.h"
#include <p101_env/resource_classes.h>
#include <p101_env/wrapper.h>

/*
 *Copyright 2021-2024 D'Arcy Smith.
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

#include <stdint.h>

static int         stdio_error_code(int error_code);
static const void *pointer_value_for_log(uintptr_t pointer_value);
static void        track_line_buffer(const struct p101_env *env, uintptr_t old_buffer, size_t old_size, const void *new_buffer, size_t new_size);

static int stdio_error_code(int error_code)
{
    if(error_code == 0)
    {
        error_code = EIO;
    }

    return error_code;
}

static const void *pointer_value_for_log(uintptr_t pointer_value)
{
#ifdef __clang_analyzer__
    (void)pointer_value;
    return NULL;
#else
    return (const void *)pointer_value;    // NOLINT(clang-analyzer-unix.Malloc,performance-no-int-to-ptr)
#endif
}

static void track_line_buffer(const struct p101_env *env, uintptr_t old_buffer, size_t old_size, const void *new_buffer, size_t new_size)
{
    const void *old_pointer;

    old_pointer = pointer_value_for_log(old_buffer);
    if(old_buffer == 0U && new_buffer != NULL)
    {
        P101_TRACK_ALLOC(env, new_buffer, new_size);
    }
    else if(old_buffer != 0U && (old_pointer != new_buffer || old_size != new_size))
    {
        P101_TRACK_REALLOC(env, old_pointer, new_buffer, new_size);
    }
}

FILE *p101_fdopen(const struct p101_env *env, struct p101_error *err, int fildes, const char *mode)
{
    FILE *ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, NULL);
    errno   = 0;
    ret_val = fdopen(fildes, mode);

    if(ret_val == NULL)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else
    {
        P101_TRACK_POINTER_RESOURCE_ACQUIRE(env, P101_RESOURCE_CLASS_STDIO_STREAM, ret_val, 0U, "fdopen");
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_fileno(const struct p101_env *env, struct p101_error *err, FILE *stream)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = fileno(stream);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

void p101_flockfile(const struct p101_env *env, FILE *file)
{
    P101_TRACE(env);
    errno = 0;
    flockfile(file);
    P101_TRACE_EXIT(env);
}

FILE *p101_fmemopen(const struct p101_env *env, struct p101_error *err, void *restrict buf, size_t size, const char *restrict mode)
{
    FILE *ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, NULL);
    errno   = 0;
    ret_val = fmemopen(buf, size, mode);

    if(ret_val == NULL)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else
    {
        P101_TRACK_POINTER_RESOURCE_ACQUIRE(env, P101_RESOURCE_CLASS_STDIO_STREAM, ret_val, 0U, "fmemopen");
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_fseeko(const struct p101_env *env, struct p101_error *err, FILE *stream, off_t offset, int whence)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = fseeko(stream, offset, whence);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

off_t p101_ftello(const struct p101_env *env, struct p101_error *err, FILE *stream)
{
    off_t ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, (off_t)-1);
    errno   = 0;
    ret_val = ftello(stream);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_ftrylockfile(const struct p101_env *env, FILE *file)
{
    int ret_val;

    P101_TRACE(env);
    errno   = 0;
    ret_val = ftrylockfile(file);

    P101_TRACE_EXIT(env);
    return ret_val;
}

void p101_funlockfile(const struct p101_env *env, FILE *file)
{
    P101_TRACE(env);
    errno = 0;
    funlockfile(file);
    P101_TRACE_EXIT(env);
}

int p101_getc_unlocked(const struct p101_env *env, struct p101_error *err, FILE *stream)
{
    int actual_error;
    int ret_val;
    int stream_error;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno = 0;
#ifdef __GNUC__
    #pragma GCC diagnostic push
    //    #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
    #pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif
    ret_val      = getc_unlocked(stream);
    actual_error = errno;
    stream_error = ferror(stream);
#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

    if(ret_val == EOF && stream_error != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, stdio_error_code(actual_error));
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_getchar_unlocked(const struct p101_env *env, struct p101_error *err)
{
    int actual_error;
    int ret_val;
    int stream_error;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno = 0;
#ifdef __GNUC__
    #pragma GCC diagnostic push
    //    #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
    #pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif
    ret_val      = getchar_unlocked();
    actual_error = errno;
    stream_error = ferror(stdin);
#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

    if(ret_val == EOF && stream_error != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, stdio_error_code(actual_error));
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

ssize_t p101_getdelim(const struct p101_env *env, struct p101_error *err, char **restrict lineptr, size_t *restrict n, int delimiter, FILE *restrict stream)
{
    int       actual_error;
    uintptr_t old_buffer;
    size_t    old_size;
    ssize_t   ret_val;
    int       stream_error;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, (ssize_t)-1);
    old_buffer   = (uintptr_t)*lineptr;
    old_size     = *n;
    errno        = 0;
    ret_val      = getdelim(lineptr, n, delimiter, stream);
    actual_error = errno;
    track_line_buffer(env, old_buffer, old_size, *lineptr, *n);
    stream_error = ferror(stream);

    if(ret_val == -1 && stream_error != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, stdio_error_code(actual_error));
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

ssize_t p101_getline(const struct p101_env *env, struct p101_error *err, char **restrict lineptr, size_t *restrict n, FILE *restrict stream)
{
    int       actual_error;
    uintptr_t old_buffer;
    size_t    old_size;
    ssize_t   ret_val;
    int       stream_error;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, (ssize_t)-1);
    old_buffer   = (uintptr_t)*lineptr;
    old_size     = *n;
    errno        = 0;
    ret_val      = getline(lineptr, n, stream);
    actual_error = errno;
    track_line_buffer(env, old_buffer, old_size, *lineptr, *n);
    stream_error = ferror(stream);

    if(ret_val == -1 && stream_error != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, stdio_error_code(actual_error));
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

FILE *p101_open_memstream(const struct p101_env *env, struct p101_error *err, char **bufp, size_t *sizep)
{
    FILE *ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, NULL);
    errno   = 0;
    ret_val = open_memstream(bufp, sizep);

    if(ret_val == NULL)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else
    {
        P101_TRACK_POINTER_RESOURCE_ACQUIRE(env, P101_RESOURCE_CLASS_STDIO_STREAM, ret_val, 0U, "open_memstream");
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_putc_unlocked(const struct p101_env *env, struct p101_error *err, int c, FILE *stream)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = putc_unlocked(c, stream);

    if(ret_val == EOF)
    {
        P101_ERROR_RAISE_ERRNO(err, stdio_error_code(errno));
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_putchar_unlocked(const struct p101_env *env, struct p101_error *err, int c)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = putchar_unlocked(c);

    if(ret_val == EOF)
    {
        P101_ERROR_RAISE_ERRNO(err, stdio_error_code(errno));
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_vdprintf(const struct p101_env *env, struct p101_error *err, int fildes, const char *restrict format, va_list ap)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    ret_val = vdprintf(fildes, format, ap);
#pragma GCC diagnostic pop

    if(ret_val < 0)
    {
        P101_ERROR_RAISE_ERRNO(err, stdio_error_code(errno));
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

/*
 * Copyright 2022-2024 D'Arcy Smith.
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

#include <limits.h>

#ifdef __linux__
    #include <stdio_ext.h>
#endif

int p101_fpurge(const struct p101_env *env, struct p101_error *err, FILE *stream)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);

#ifdef __linux__
    // glibc has no fpurge(); it spells it __fpurge() in <stdio_ext.h>, which
    // returns void and cannot fail. The BSD/macOS fpurge() can fail with EBADF.
    __fpurge(stream);
    (void)err;
    ret_val = 0;
#else
    ret_val = fpurge(stream);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
#endif

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_setbuffer(const struct p101_env *env, struct p101_error *err, FILE *stream, char *buf, size_t size)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    ret_val = 0;
#if defined(__APPLE__) || defined(__FreeBSD__)
    if(size > (size_t)INT_MAX)
    {
        P101_ERROR_RAISE_ERRNO(err, ERANGE);
        ret_val = -1;
    }
    else
    {
        setbuffer(stream, buf, (int)size);
    }
#else
    setbuffer(stream, buf, size);
#endif

    P101_WRAPPER_DONE(env);
    return ret_val;
}

void p101_setlinebuf(const struct p101_env *env, FILE *stream)
{
    P101_TRACE(env);
    setlinebuf(stream);
    P101_TRACE_EXIT(env);
}
