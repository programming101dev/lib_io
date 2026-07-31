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
#include <limits.h>
#include <p101_env/wrapper.h>

#ifdef __linux__
    #include <stdio_ext.h>
#endif

int p101_fpurge(const struct p101_env *env, struct p101_error *err, FILE *stream)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, -1);

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

    P101_TRACE_EXIT(env);
    return ret_val;
}

int p101_setbuffer(const struct p101_env *env, struct p101_error *err, FILE *stream, char *buf, size_t size)
{
    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, -1);
#if defined(__APPLE__) || defined(__FreeBSD__)
    if(size > (size_t)INT_MAX)
    {
        P101_ERROR_RAISE_ERRNO(err, ERANGE);
        P101_TRACE_EXIT(env);
        return -1;
    }
    setbuffer(stream, buf, (int)size);
#else
    setbuffer(stream, buf, size);
#endif
    P101_TRACE_EXIT(env);
    return 0;
}

void p101_setlinebuf(const struct p101_env *env, FILE *stream)
{
    P101_TRACE(env);
    setlinebuf(stream);
    P101_TRACE_EXIT(env);
}
