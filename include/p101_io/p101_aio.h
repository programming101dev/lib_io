#ifndef LIBP101_IO_P101_AIO_H
#define LIBP101_IO_P101_AIO_H

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

    int     p101_aio_cancel(const struct p101_env *env, struct p101_error *err, int fildes, struct aiocb *aiocbp);
    int     p101_aio_error(const struct p101_env *env, struct p101_error *err, const struct aiocb *aiocbp);
    int     p101_aio_fsync(const struct p101_env *env, struct p101_error *err, int op, struct aiocb *aiocbp);
    int     p101_aio_read(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp);
    ssize_t p101_aio_return(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp);
    int     p101_aio_suspend(const struct p101_env *env, struct p101_error *err, const struct aiocb *const list[], int nent, const struct timespec *timeout);
    int     p101_aio_write(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp);
    int     p101_lio_listio(const struct p101_env *env, struct p101_error *err, int mode, struct aiocb *restrict const list[restrict], int nent, struct sigevent *restrict sig);

#ifdef __cplusplus
}
#endif

#endif    // LIBP101_IO_P101_AIO_H
