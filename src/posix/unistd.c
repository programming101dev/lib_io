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

#include "p101_io/io.h"
#include <p101_env/wrapper.h>

int p101_close(const struct p101_env *env, struct p101_error *err, int fildes)
{
    errno_t actual_error;
    int     ret_val;
    int     fault;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, "close");

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        return -1;
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

    return ret_val;
}

int p101_dup(const struct p101_env *env, struct p101_error *err, int fildes)
{
    int ret_val;
    int fault;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, "dup");

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        return -1;
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

    return ret_val;
}

int p101_dup2(const struct p101_env *env, struct p101_error *err, int fildes, int fildes2)
{
    int ret_val;
    int fault;

    P101_TRACE(env);
    fault = p101_env_check_fault(env, "dup2");

    if(fault != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, fault);
        P101_TRACE_EXIT(env);

        return -1;
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

    return ret_val;
}

off_t p101_lseek(const struct p101_env *env, struct p101_error *err, int fildes, off_t offset, int whence)
{
    off_t ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, (off_t)-1);
    errno   = 0;
    ret_val = lseek(fildes, offset, whence);

    if(ret_val == (off_t)-1)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_TRACE_EXIT(env);
    return ret_val;
}

ssize_t p101_pread(const struct p101_env *env, struct p101_error *err, int fildes, void *buf, size_t nbyte, off_t offset)
{
    ssize_t                      ret_val;
    struct p101_env_fault_action fault;
    int                          hide_success;

    P101_TRACE(env);
    if(p101_env_check_fault_action(env, "pread", &fault))
    {
        if(fault.kind == P101_ENV_FAULT_ERROR)
        {
            P101_ERROR_RAISE_ERRNO(err, fault.errnum);
            P101_TRACE_EXIT(env);
            return -1;
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
        p101_env_record_fault_action(env, "pread", &fault);
    }
    else if(hide_success)
    {
        p101_env_record_fault_action(env, "pread", &fault);
        P101_ERROR_RAISE_ERRNO(err, fault.errnum);
        ret_val = -1;
    }

    P101_TRACE_EXIT(env);
    return ret_val;
}

ssize_t p101_pwrite(const struct p101_env *env, struct p101_error *err, int fildes, const void *buf, size_t nbyte, off_t offset)
{
    ssize_t                      ret_val;
    struct p101_env_fault_action fault;
    int                          hide_success;

    P101_TRACE(env);
    if(p101_env_check_fault_action(env, "pwrite", &fault))
    {
        if(fault.kind == P101_ENV_FAULT_ERROR)
        {
            P101_ERROR_RAISE_ERRNO(err, fault.errnum);
            P101_TRACE_EXIT(env);
            return -1;
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
        p101_env_record_fault_action(env, "pwrite", &fault);
    }
    else if(hide_success)
    {
        p101_env_record_fault_action(env, "pwrite", &fault);
        P101_ERROR_RAISE_ERRNO(err, fault.errnum);
        ret_val = -1;
    }

    P101_TRACE_EXIT(env);
    return ret_val;
}

ssize_t p101_read(const struct p101_env *env, struct p101_error *err, int fildes, void *buf, size_t nbyte)
{
    ssize_t                      ret_val;
    struct p101_env_fault_action fault;
    int                          hide_success;

    P101_TRACE(env);
    if(p101_env_check_fault_action(env, "read", &fault))
    {
        if(fault.kind == P101_ENV_FAULT_ERROR)
        {
            P101_ERROR_RAISE_ERRNO(err, fault.errnum);
            P101_TRACE_EXIT(env);
            return -1;
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
        p101_env_record_fault_action(env, "read", &fault);
    }
    else if(hide_success)
    {
        p101_env_record_fault_action(env, "read", &fault);
        P101_ERROR_RAISE_ERRNO(err, fault.errnum);
        ret_val = -1;
    }

    P101_TRACE_EXIT(env);

    return ret_val;
}

ssize_t p101_write(const struct p101_env *env, struct p101_error *err, int fildes, const void *buf, size_t nbyte)
{
    ssize_t                      ret_val;
    struct p101_env_fault_action fault;
    int                          hide_success;

    P101_TRACE(env);
    if(p101_env_check_fault_action(env, "write", &fault))
    {
        if(fault.kind == P101_ENV_FAULT_ERROR)
        {
            P101_ERROR_RAISE_ERRNO(err, fault.errnum);
            P101_TRACE_EXIT(env);
            return -1;
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
        p101_env_record_fault_action(env, "write", &fault);
    }
    else if(hide_success)
    {
        p101_env_record_fault_action(env, "write", &fault);
        P101_ERROR_RAISE_ERRNO(err, fault.errnum);
        ret_val = -1;
    }

    P101_TRACE_EXIT(env);

    return ret_val;
}
