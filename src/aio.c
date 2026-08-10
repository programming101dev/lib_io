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

#include "p101_io/p101_aio.h"
#include <p101_env/wrapper.h>

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
    if(aiocbp != NULL && aiocbp->aio_fildes < 0)
    {
        errno   = EBADF;
        ret_val = -1;
        P101_ERROR_RAISE_ERRNO(err, EBADF);
    }
    else
    {
        errno   = 0;
        ret_val = aio_read(aiocbp);

        if(ret_val == -1)
        {
            P101_ERROR_RAISE_ERRNO(err, errno);
        }
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
    if(nent < 0)
    {
        errno   = EINVAL;
        ret_val = -1;
        P101_ERROR_RAISE_ERRNO(err, EINVAL);
    }
    else
    {
        errno   = 0;
        ret_val = aio_suspend(list, nent, timeout);

        if(ret_val == -1)
        {
            P101_ERROR_RAISE_ERRNO(err, errno);
        }
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_aio_write(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    if(aiocbp != NULL && aiocbp->aio_fildes < 0)
    {
        errno   = EBADF;
        ret_val = -1;
        P101_ERROR_RAISE_ERRNO(err, EBADF);
    }
    else
    {
        errno   = 0;
        ret_val = aio_write(aiocbp);

        if(ret_val == -1)
        {
            P101_ERROR_RAISE_ERRNO(err, errno);
        }
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}

int p101_lio_listio(const struct p101_env *env, struct p101_error *err, int mode, struct aiocb *restrict const list[restrict], int nent, struct sigevent *restrict sig)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, ret_val, -1);
    if((mode != LIO_WAIT && mode != LIO_NOWAIT) || nent < 0)
    {
        errno   = EINVAL;
        ret_val = -1;
        P101_ERROR_RAISE_ERRNO(err, EINVAL);
    }
    else
    {
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
    }

    P101_WRAPPER_DONE(env);
    return ret_val;
}
