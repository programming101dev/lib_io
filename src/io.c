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

#include "p101_io/io.h"
#include <p101_env/wrapper.h>

int p101_aio_cancel(const struct p101_env *env, struct p101_error *err, int fildes, struct aiocb *aiocbp)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = aio_cancel(fildes, aiocbp);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_aio_error(const struct p101_env *env, struct p101_error *err, const struct aiocb *aiocbp)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN_CODE(env, err, ret_val);
    errno   = 0;
    ret_val = aio_error(aiocbp);

    if(ret_val == EINVAL || ret_val == ENOSYS)
    {
        P101_ERROR_RAISE_ERRNO(err, ret_val);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_aio_fsync(const struct p101_env *env, struct p101_error *err, int op, struct aiocb *aiocbp)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = aio_fsync(op, aiocbp);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_aio_read(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = aio_read(aiocbp);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

ssize_t p101_aio_return(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp)
{
    ssize_t ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, (ssize_t)-1);
    errno   = 0;
    ret_val = aio_return(aiocbp);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_aio_suspend(const struct p101_env *env, struct p101_error *err, const struct aiocb *const list[], int nent, const struct timespec *timeout)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = aio_suspend(list, nent, timeout);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_aio_write(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = aio_write(aiocbp);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_lio_listio(const struct p101_env *env, struct p101_error *err, int mode, struct aiocb *restrict const list[restrict], int nent, struct sigevent *restrict sig)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
#endif
    ret_val = lio_listio(mode, list, nent, sig);
#pragma GCC diagnostic pop

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

/*
 * Copyright 2021-2024 D'Arcy Smith.
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

#include <fcntl.h>
#include <stdarg.h>

static int    fcntl_makes_fd(int cmd);
static int    fcntl_uses_int_arg(int cmd);
static int    fcntl_uses_flock_arg(int cmd);
static int    open_uses_mode_arg(int oflag);
static mode_t open_mode_arg(va_list *args);

/*
 * Most fcntl() commands return flags, a count, or 0. Only the duplicating
 * commands return a brand new descriptor, and only those may go into the
 * fd ledger.
 */
static int fcntl_makes_fd(int cmd)
{
    int makes_fd;

    makes_fd = (cmd == F_DUPFD);

#ifdef F_DUPFD_CLOEXEC
    if(cmd == F_DUPFD_CLOEXEC)
    {
        makes_fd = 1;
    }
#endif

    return makes_fd;
}

static int fcntl_uses_int_arg(int cmd)
{
    int uses_arg;

    uses_arg = (cmd == F_DUPFD || cmd == F_SETFD || cmd == F_SETFL);

#ifdef F_DUPFD_CLOEXEC
    if(cmd == F_DUPFD_CLOEXEC)
    {
        uses_arg = 1;
    }
#endif

#ifdef F_SETOWN
    if(cmd == F_SETOWN)
    {
        uses_arg = 1;
    }
#endif

    return uses_arg;
}

static int fcntl_uses_flock_arg(int cmd)
{
    int uses_arg;

    uses_arg = (cmd == F_GETLK || cmd == F_SETLK || cmd == F_SETLKW);

    return uses_arg;
}

static int open_uses_mode_arg(int oflag)
{
    int uses_arg;

    uses_arg = ((oflag & O_CREAT) == O_CREAT);

#ifdef O_TMPFILE
    if((oflag & O_TMPFILE) == O_TMPFILE)
    {
        uses_arg = 1;
    }
#endif

    return uses_arg;
}

static mode_t open_mode_arg(va_list *args)
{
    mode_t mode;

#if defined(__APPLE__) || defined(__FreeBSD__)
    mode = (mode_t)va_arg(*args, int);
#else
    mode = va_arg(*args, mode_t);
#endif

    return mode;
}

int p101_creat(const struct p101_env *env, struct p101_error *err, const char *path, mode_t mode)
{
    int p101_single_result_;
    int ret_val;
    int fault;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, __func__);

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    errno   = 0;
    ret_val = creat(path, mode);    // NOLINT(android-cloexec-creat)

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else
    {
        P101_TRACK_OPEN(env, ret_val);
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

int p101_fcntl(const struct p101_env *env, struct p101_error *err, int fildes, int cmd, ...)
{
    int     p101_single_result_;
    int     ret_val;
    int     fault;
    int     uses_int_arg;
    int     uses_flock_arg;
    int     makes_fd;
    va_list args;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, __func__);

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    errno = 0;
    va_start(args, cmd);
    uses_int_arg   = fcntl_uses_int_arg(cmd);
    uses_flock_arg = fcntl_uses_flock_arg(cmd);

    if(uses_int_arg)
    {
        int arg;

        arg     = va_arg(args, int);
        ret_val = fcntl(fildes, cmd, arg);
    }
    else if(uses_flock_arg)
    {
        struct flock *arg;

        arg     = va_arg(args, struct flock *);
        ret_val = fcntl(fildes, cmd, arg);
    }
    else
    {
        ret_val = fcntl(fildes, cmd);
    }

    va_end(args);

    makes_fd = fcntl_makes_fd(cmd);
    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else if(makes_fd)
    {
        P101_TRACK_OPEN(env, ret_val);
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

int p101_open(const struct p101_env *env, struct p101_error *err, const char *path, int oflag, ...)
{
    int     p101_single_result_;
    int     ret_val;
    int     fault;
    int     uses_mode;
    va_list args;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, __func__);

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    errno     = 0;
    uses_mode = open_uses_mode_arg(oflag);

    if(uses_mode)
    {
        mode_t mode;

        va_start(args, oflag);
        mode = open_mode_arg(&args);
        va_end(args);

        ret_val = open(path, oflag, mode);
    }
    else
    {
        ret_val = open(path, oflag);
    }

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else
    {
        P101_TRACK_OPEN(env, ret_val);
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

int p101_openat(const struct p101_env *env, struct p101_error *err, int fd, const char *path, int oflag, ...)
{
    int     p101_single_result_;
    int     ret_val;
    int     fault;
    int     uses_mode;
    va_list args;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, __func__);

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    errno     = 0;
    uses_mode = open_uses_mode_arg(oflag);

    if(uses_mode)
    {
        mode_t mode;

        va_start(args, oflag);
        mode = open_mode_arg(&args);
        va_end(args);

        ret_val = openat(fd, path, oflag, mode);
    }
    else
    {
        ret_val = openat(fd, path, oflag);
    }

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else
    {
        P101_TRACK_OPEN(env, ret_val);
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

/*
 * Copyright 2021-2024 D'Arcy Smith.
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

int p101_poll(const struct p101_env *env, struct p101_error *err, struct pollfd fds[], nfds_t nfds, int timeout)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = poll(fds, nfds, timeout);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

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
        P101_TRACK_POINTER_RESOURCE_ACQUIRE(env, "stdio-stream", ret_val, 0U, "fdopen");
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
        P101_TRACK_POINTER_RESOURCE_ACQUIRE(env, "stdio-stream", ret_val, 0U, "fmemopen");
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
        P101_TRACK_POINTER_RESOURCE_ACQUIRE(env, "stdio-stream", ret_val, 0U, "open_memstream");
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
 * Copyright 2021-2024 D'Arcy Smith.
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

int p101_pselect(const struct p101_env *env, struct p101_error *err, int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, const struct timespec *restrict timeout, const sigset_t *restrict sigmask)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = pselect(nfds, readfds, writefds, errorfds, timeout, sigmask);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_select(const struct p101_env *env, struct p101_error *err, int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, struct timeval *restrict timeout)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = select(nfds, readfds, writefds, errorfds, timeout);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

/*
 * Copyright 2021-2024 D'Arcy Smith.
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

int p101_close(const struct p101_env *env, struct p101_error *err, int fildes)
{
    int     p101_single_result_;
    errno_t actual_error;
    int     ret_val;
    int     fault;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, __func__);

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    errno        = 0;
    ret_val      = close(fildes);
    actual_error = errno;

    if(ret_val == -1)
    {
        if(actual_error == EBADF)
        {
            P101_TRACK_CLOSE(env, fildes);
        }
        P101_ERROR_RAISE_ERRNO(err, actual_error);
    }
    else
    {
        P101_TRACK_CLOSE(env, fildes);
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

int p101_dup(const struct p101_env *env, struct p101_error *err, int fildes)
{
    int p101_single_result_;
    int ret_val;
    int fault;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, __func__);

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    errno   = 0;
    ret_val = dup(fildes);    // NOLINT(android-cloexec-dup)

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else
    {
        P101_TRACK_OPEN(env, ret_val);
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

int p101_dup2(const struct p101_env *env, struct p101_error *err, int fildes, int fildes2)
{
    int p101_single_result_;
    int ret_val;
    int fault;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, __func__);

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        p101_single_result_ = -1;
        goto p101_single_exit_;
    }

    errno   = 0;
    ret_val = dup2(fildes, fildes2);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else
    {
        /* dup2() SILENTLY closes fildes2 if it was already open, so the
         * ledger retires the old entry before recording the new one.
         * Retiring an untracked descriptor is a no-op, and when
         * fildes == fildes2 dup2() does nothing -- the net effect here is
         * still one tracked descriptor, now attributed to this call site. */
        P101_TRACK_CLOSE(env, fildes2);
        P101_TRACK_OPEN(env, ret_val);
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

off_t p101_lseek(const struct p101_env *env, struct p101_error *err, int fildes, off_t offset, int whence)
{
    off_t ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, (off_t)-1);
    errno   = 0;
    ret_val = lseek(fildes, offset, whence);

    if(ret_val == (off_t)-1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

ssize_t p101_pread(const struct p101_env *env, struct p101_error *err, int fildes, void *buf, size_t nbyte, off_t offset)
{
    ssize_t                      p101_single_result_;
    ssize_t                      ret_val;
    struct p101_env_fault_action fault;
    int                          hide_success;
    int                          has_fault;

    P101_TRACE(env);
    has_fault = p101_env_check_fault_action(env, __func__, &fault);
    if(has_fault)
    {
        if(fault.kind == P101_ENV_FAULT_ERROR)
        {
            P101_ERROR_RAISE_ERRNO(err, fault.errnum);
            P101_TRACE_EXIT(env);
            p101_single_result_ = -1;
            goto p101_single_exit_;
        }
        if(fault.kind == P101_ENV_FAULT_SHORT)
        {
            nbyte = p101_wrapper_short_count(nbyte, fault.amount);
        }
    }
    hide_success = fault.kind == P101_ENV_FAULT_UNCERTAIN;
    errno        = 0;
    ret_val      = pread(fildes, buf, nbyte, offset);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else if(fault.kind == P101_ENV_FAULT_SHORT && ret_val > 0)
    {
        p101_env_record_fault_action(env, __func__, &fault);
    }
    else if(hide_success)
    {
        p101_env_record_fault_action(env, __func__, &fault);
        P101_ERROR_RAISE_ERRNO(err, fault.errnum);
        ret_val = -1;
    }

    P101_TRACE_EXIT(env);
    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

ssize_t p101_pwrite(const struct p101_env *env, struct p101_error *err, int fildes, const void *buf, size_t nbyte, off_t offset)
{
    ssize_t                      p101_single_result_;
    ssize_t                      ret_val;
    struct p101_env_fault_action fault;
    int                          hide_success;
    int                          has_fault;

    P101_TRACE(env);
    has_fault = p101_env_check_fault_action(env, __func__, &fault);
    if(has_fault)
    {
        if(fault.kind == P101_ENV_FAULT_ERROR)
        {
            P101_ERROR_RAISE_ERRNO(err, fault.errnum);
            P101_TRACE_EXIT(env);
            p101_single_result_ = -1;
            goto p101_single_exit_;
        }
        if(fault.kind == P101_ENV_FAULT_SHORT)
        {
            nbyte = p101_wrapper_short_count(nbyte, fault.amount);
        }
    }
    hide_success = fault.kind == P101_ENV_FAULT_UNCERTAIN;
    errno        = 0;
    ret_val      = pwrite(fildes, buf, nbyte, offset);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else if(fault.kind == P101_ENV_FAULT_SHORT && ret_val > 0)
    {
        p101_env_record_fault_action(env, __func__, &fault);
    }
    else if(hide_success)
    {
        p101_env_record_fault_action(env, __func__, &fault);
        P101_ERROR_RAISE_ERRNO(err, fault.errnum);
        ret_val = -1;
    }

    P101_TRACE_EXIT(env);
    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

ssize_t p101_read(const struct p101_env *env, struct p101_error *err, int fildes, void *buf, size_t nbyte)
{
    ssize_t                      p101_single_result_;
    ssize_t                      ret_val;
    struct p101_env_fault_action fault;
    int                          hide_success;
    int                          has_fault;

    P101_TRACE(env);
    has_fault = p101_env_check_fault_action(env, __func__, &fault);
    if(has_fault)
    {
        if(fault.kind == P101_ENV_FAULT_ERROR)
        {
            P101_ERROR_RAISE_ERRNO(err, fault.errnum);
            P101_TRACE_EXIT(env);
            p101_single_result_ = -1;
            goto p101_single_exit_;
        }
        if(fault.kind == P101_ENV_FAULT_SHORT)
        {
            nbyte = p101_wrapper_short_count(nbyte, fault.amount);
        }
    }
    hide_success = fault.kind == P101_ENV_FAULT_UNCERTAIN;

    errno   = 0;
    ret_val = read(fildes, buf, nbyte);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else if(fault.kind == P101_ENV_FAULT_SHORT && ret_val > 0)
    {
        p101_env_record_fault_action(env, __func__, &fault);
    }
    else if(hide_success)
    {
        p101_env_record_fault_action(env, __func__, &fault);
        P101_ERROR_RAISE_ERRNO(err, fault.errnum);
        ret_val = -1;
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

ssize_t p101_write(const struct p101_env *env, struct p101_error *err, int fildes, const void *buf, size_t nbyte)
{
    ssize_t                      p101_single_result_;
    ssize_t                      ret_val;
    struct p101_env_fault_action fault;
    int                          hide_success;
    int                          has_fault;

    P101_TRACE(env);
    has_fault = p101_env_check_fault_action(env, __func__, &fault);
    if(has_fault)
    {
        if(fault.kind == P101_ENV_FAULT_ERROR)
        {
            P101_ERROR_RAISE_ERRNO(err, fault.errnum);
            P101_TRACE_EXIT(env);
            p101_single_result_ = -1;
            goto p101_single_exit_;
        }
        if(fault.kind == P101_ENV_FAULT_SHORT)
        {
            nbyte = p101_wrapper_short_count(nbyte, fault.amount);
        }
    }
    hide_success = fault.kind == P101_ENV_FAULT_UNCERTAIN;

    errno   = 0;
    ret_val = write(fildes, buf, nbyte);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }
    else if(fault.kind == P101_ENV_FAULT_SHORT && ret_val > 0)
    {
        p101_env_record_fault_action(env, __func__, &fault);
    }
    else if(hide_success)
    {
        p101_env_record_fault_action(env, __func__, &fault);
        P101_ERROR_RAISE_ERRNO(err, fault.errnum);
        ret_val = -1;
    }

    P101_TRACE_EXIT(env);

    p101_single_result_ = ret_val;
    goto p101_single_exit_;

p101_single_exit_:
    return p101_single_result_;
}

ssize_t p101_readv(const struct p101_env *env, struct p101_error *err, int fildes, const struct iovec *iov, int iovcnt)
{
    ssize_t ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = readv(fildes, iov, iovcnt);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

ssize_t p101_writev(const struct p101_env *env, struct p101_error *err, int fildes, const struct iovec *iov, int iovcnt)
{
    ssize_t ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = writev(fildes, iov, iovcnt);

    if(ret_val == -1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

#ifdef __linux__
    #include <crypt.h>
#endif
#include <unistd.h>

int p101_lockf(const struct p101_env *env, struct p101_error *err, int fildes, int function, off_t size)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    errno   = 0;
    ret_val = lockf(fildes, function, size);

    if(ret_val == -1 && errno != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
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
