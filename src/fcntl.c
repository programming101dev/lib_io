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

#include "p101_io/p101_fcntl.h"
#include <p101_env/wrapper.h>

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
