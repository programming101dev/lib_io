#ifndef LIBP101_IO_P101_FCNTL_H
#define LIBP101_IO_P101_FCNTL_H

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

    int p101_creat(const struct p101_env *env, struct p101_error *err, const char *path, mode_t mode);
    int p101_fcntl(const struct p101_env *env, struct p101_error *err, int fildes, int cmd, ...);
    int p101_open(const struct p101_env *env, struct p101_error *err, const char *path, int oflag, ...) P101_ATTR_SEMANTIC_ROLE("p101:filesystem:path-use");
    int p101_openat(const struct p101_env *env, struct p101_error *err, int fd, const char *path, int oflag, ...) P101_ATTR_SEMANTIC_ROLE("p101:filesystem:path-use");

#ifdef __cplusplus
}
#endif

#endif    // LIBP101_IO_P101_FCNTL_H
