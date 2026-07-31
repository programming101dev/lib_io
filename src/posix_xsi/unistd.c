#include "p101_io/io.h"
#include <p101_env/wrapper.h>
#ifdef __linux__
    #include <crypt.h>
#endif
#include <unistd.h>

int p101_lockf(const struct p101_env *env, struct p101_error *err, int fildes, int function, off_t size)
{
    int ret_val;

    P101_TRACE(env);
    P101_WRAPPER_FAULT_RETURN(env, err, -1);
    errno   = 0;
    ret_val = lockf(fildes, function, size);

    if(ret_val == -1 && errno != 0)
    {
        P101_ERROR_RAISE_ERRNO(err, errno);
    }

    P101_TRACE_EXIT(env);
    return ret_val;
}
