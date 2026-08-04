#include "p101_io/io.h"
#include <p101_env/wrapper.h>

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
