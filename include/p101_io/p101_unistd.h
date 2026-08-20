#ifndef LIBP101_IO_P101_UNISTD_H
#define LIBP101_IO_P101_UNISTD_H

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

    int     p101_close(const struct p101_env *env, struct p101_error *err, int fildes) P101_ATTR_SEMANTIC_ROLE("p101:progress:uncertain");
    int     p101_dup(const struct p101_env *env, struct p101_error *err, int fildes);
    int     p101_dup2(const struct p101_env *env, struct p101_error *err, int fildes, int fildes2);
    int     p101_lockf(const struct p101_env *env, struct p101_error *err, int fildes, int function, off_t size);
    off_t   p101_lseek(const struct p101_env *env, struct p101_error *err, int fildes, off_t offset, int whence);
    ssize_t p101_pread(const struct p101_env *env, struct p101_error *err, int fildes, void *buf, size_t nbyte, off_t offset) P101_ATTR_SEMANTIC_ROLE("p101:result:partial");
    ssize_t p101_pwrite(const struct p101_env *env, struct p101_error *err, int fildes, const void *buf, size_t nbyte, off_t offset) P101_ATTR_SEMANTIC_ROLE("p101:result:partial");
    ssize_t p101_read(const struct p101_env *env, struct p101_error *err, int fildes, void *buf, size_t nbyte) P101_ATTR_SEMANTIC_ROLE("p101:result:partial");
    ssize_t p101_write(const struct p101_env *env, struct p101_error *err, int fildes, const void *buf, size_t nbyte) P101_ATTR_SEMANTIC_ROLE("p101:result:partial");

#ifdef __cplusplus
}
#endif

#endif    // LIBP101_IO_P101_UNISTD_H
