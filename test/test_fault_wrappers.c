#include <errno.h>
#include <p101_env/env.h>
#include <p101_error/error.h>
#include <p101_io/io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define EXPECT(condition)                                                                                                                                                                                                                                          \
    do                                                                                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                                              \
        if(!(condition))                                                                                                                                                                                                                                           \
        {                                                                                                                                                                                                                                                          \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                                                                                                                                                                                   \
            failures++;                                                                                                                                                                                                                                            \
        }                                                                                                                                                                                                                                                          \
    } while(0)

struct fault_state
{
    int checks;
    int errnum;
};

static int fail_next_call(const struct p101_env *env, const char *call_name, void *user_data)
{
    struct fault_state *state;

    (void)env;
    (void)call_name;
    state = user_data;
    state->checks++;
    return state->errnum;
}

/* P101_TEST_CASE(p101_aio_cancel) */
static void test_p101_aio_cancel(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF, ENOSYS};
#elif defined(__APPLE__)
    static const int errors[] = {EBADF};
#elif defined(__FreeBSD__)
    static const int errors[] = {EBADF};
#else
    static const int errors[] = {EBADF};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_cancel(env, err, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_error) */
static void test_p101_aio_error(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EINVAL, ENOSYS};
#elif defined(__APPLE__)
    static const int errors[] = {EINVAL};
#elif defined(__FreeBSD__)
    static const int errors[] = {EINVAL};
#else
    static const int errors[] = {EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_error(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_fsync) */
static void test_p101_aio_fsync(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EAGAIN, EBADF, EINVAL, ENOSYS};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, EINVAL};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EINVAL, EOPNOTSUPP};
#else
    static const int errors[] = {EAGAIN, EBADF, EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_fsync(env, err, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_read) */
static void test_p101_aio_read(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EAGAIN, EBADF, EINVAL, ENOSYS, EOVERFLOW};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, ECANCELED, EFAULT, EINVAL, ENOSYS, EOVERFLOW};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, ECANCELED, EFAULT, EINVAL, EOPNOTSUPP, EOVERFLOW};
#else
    static const int errors[] = {EAGAIN, EBADF, ECANCELED, EINVAL, EOVERFLOW};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_read(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_return) */
static void test_p101_aio_return(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EINVAL, ENOSYS};
#elif defined(__APPLE__)
    static const int errors[] = {EINPROGRESS, EINVAL};
#elif defined(__FreeBSD__)
    static const int errors[] = {EINVAL};
#else
    static const int errors[] = {EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_aio_return(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_suspend) */
static void test_p101_aio_suspend(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EAGAIN, EINTR, ENOSYS};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EINTR, EINVAL};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EINTR, EINVAL};
#else
    static const int errors[] = {EAGAIN, EINTR};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_suspend(env, err, NULL, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_write) */
static void test_p101_aio_write(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EAGAIN, EBADF, EFBIG, EINVAL, ENOSYS};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, ECANCELED, EFAULT, EINVAL, ENOSYS};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, ECANCELED, EFAULT, EINVAL, EOPNOTSUPP};
#else
    static const int errors[] = {EAGAIN, EBADF, ECANCELED, EFBIG, EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_write(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_close) */
static void test_p101_close(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF, EINTR, EIO, ENOSPC};
#elif defined(__APPLE__)
    static const int errors[] = {EBADF, EINTR, EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EBADF, EINTR, ENOSPC};
#else
    static const int errors[] = {EBADF, EINPROGRESS, EINTR, EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_close(env, err, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_creat) */
static void test_p101_creat(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EACCES, EBUSY, EDQUOT, EEXIST, EFAULT, EFBIG, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENODEV, ENOENT, ENOMEM, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EPERM, EROFS, ETXTBSY, EWOULDBLOCK};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_creat(env, err, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_dup) */
static void test_p101_dup(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF, EBUSY, EMFILE, ENOMEM};
#elif defined(__APPLE__)
    static const int errors[] = {EBADF, EINTR, EMFILE};
#elif defined(__FreeBSD__)
    static const int errors[] = {EBADF, EMFILE};
#else
    static const int errors[] = {EBADF, EMFILE};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_dup(env, err, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_dup2) */
static void test_p101_dup2(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF, EBUSY, EINTR, EMFILE, ENOMEM};
#elif defined(__APPLE__)
    static const int errors[] = {EBADF, EINTR, EMFILE};
#elif defined(__FreeBSD__)
    static const int errors[] = {EBADF};
#else
    static const int errors[] = {EBADF, EINTR, EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_dup2(env, err, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fcntl) */
static void test_p101_fcntl(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EACCES, EAGAIN, EBADF, EINVAL};
#elif defined(__APPLE__)
    static const int errors[] = {EACCES, EAGAIN, EBADF, EDEADLK, EFBIG, EINTR, EINVAL, EMFILE, ENOLCK, ENOSPC, ENOTSUP, EOVERFLOW, EPERM, ESRCH, EXDEV};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EBUSY, EDEADLK, EINTR, EINVAL, EMFILE, ENOLCK, ENOTTY, EOPNOTSUPP, EOVERFLOW, EPERM, ESRCH};
#else
    static const int errors[] = {EACCES, EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, EMFILE, ENOLCK, EOVERFLOW, EPERM, ESRCH};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_fcntl(env, err, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fdopen) */
static void test_p101_fdopen(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EACCES, EAGAIN, EBADF, EINVAL, ENOMEM};
#elif defined(__APPLE__)
    static const int errors[] = {EACCES, EAGAIN, EBADF, EDEADLK, EFBIG, EINTR, EINVAL, EMFILE, ENOLCK, ENOSPC, ENOTSUP, EOVERFLOW, EPERM, ESRCH, EXDEV};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EBUSY, EDEADLK, EINTR, EINVAL, EMFILE, ENOLCK, ENOTTY, EOPNOTSUPP, EOVERFLOW, EPERM, ESRCH};
#else
    static const int errors[] = {EBADF, EINVAL, EMFILE, ENOMEM};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        FILE *result = p101_fdopen(env, err, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fileno) */
static void test_p101_fileno(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EBADF};
#else
    static const int errors[] = {EBADF};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_fileno(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fmemopen) */
static void test_p101_fmemopen(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EINVAL};
#elif defined(__FreeBSD__)
    static const int errors[] = {EINVAL};
#else
    static const int errors[] = {EINVAL, EMFILE, ENOMEM};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        FILE *result = p101_fmemopen(env, err, NULL, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fpurge) */
static void test_p101_fpurge(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_fpurge(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fseeko) */
static void test_p101_fseeko(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENETDOWN, ENETUNREACH, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE, EWOULDBLOCK};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE};
#else
    static const int errors[] = {EAGAIN, EBADF, EFBIG, EINTR, EINVAL, EIO, ENOMEM, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_fseeko(env, err, NULL, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_ftello) */
static void test_p101_ftello(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENETDOWN, ENETUNREACH, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE, EWOULDBLOCK};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE};
#else
    static const int errors[] = {EBADF, EOVERFLOW, ESPIPE};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        off_t result = p101_ftello(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_getc_unlocked) */
static void test_p101_getc_unlocked(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_getc_unlocked(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_getchar_unlocked) */
static void test_p101_getchar_unlocked(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_getchar_unlocked(env, err);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_getdelim) */
static void test_p101_getdelim(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EINVAL, ENOMEM};
#elif defined(__APPLE__)
    static const int errors[] = {EINVAL, EOVERFLOW};
#elif defined(__FreeBSD__)
    static const int errors[] = {EINVAL, EOVERFLOW};
#else
    static const int errors[] = {EINVAL, ENOMEM, EOVERFLOW};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_getdelim(env, err, NULL, NULL, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_getline) */
static void test_p101_getline(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EINVAL, ENOMEM};
#elif defined(__APPLE__)
    static const int errors[] = {EINVAL, EOVERFLOW};
#elif defined(__FreeBSD__)
    static const int errors[] = {EINVAL, EOVERFLOW};
#else
    static const int errors[] = {EINVAL, ENOMEM, EOVERFLOW};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_getline(env, err, NULL, NULL, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_lio_listio) */
static void test_p101_lio_listio(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EAGAIN, EINTR, EINVAL, EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EINTR, EINVAL, EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EINTR, EINVAL, EIO};
#else
    static const int errors[] = {EAGAIN, EINPROGRESS, EINTR, EINVAL, EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_lio_listio(env, err, 0, NULL, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_lockf) */
static void test_p101_lockf(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EACCES, EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, ENOLCK};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, ENOLCK, EOPNOTSUPP};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, ENOLCK};
#else
    static const int errors[] = {EACCES, EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, ENOLCK, EOPNOTSUPP, EOVERFLOW};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_lockf(env, err, 0, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_lseek) */
static void test_p101_lseek(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF, EINVAL, ENXIO, EOVERFLOW, ESPIPE};
#elif defined(__APPLE__)
    static const int errors[] = {EBADF, EINVAL, ENXIO, EOVERFLOW, ESPIPE};
#elif defined(__FreeBSD__)
    static const int errors[] = {EBADF, EINVAL, ENXIO, EOVERFLOW, ESPIPE};
#else
    static const int errors[] = {EBADF, EINVAL, ENXIO, EOVERFLOW, ESPIPE};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        off_t result = p101_lseek(env, err, 0, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_open) */
static void test_p101_open(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EACCES, EBUSY, EDQUOT, EEXIST, EFAULT, EFBIG, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENODEV, ENOENT, ENOMEM, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EPERM, EROFS, ETXTBSY, EWOULDBLOCK};
#elif defined(__APPLE__)
    static const int errors[] = {EACCES, EAGAIN, EBADF, EDEADLK, EDQUOT, EEXIST, EFAULT, EILSEQ, EINTR, EINVAL, EIO, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENOENT, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EROFS, ETXTBSY, EWOULDBLOCK};
#elif defined(__FreeBSD__)
    static const int errors[] = {EACCES, EBADF, EDQUOT, EEXIST, EFAULT, EINTR, EINVAL, EIO, EISDIR, ELOOP, EMFILE, EMLINK, ENAMETOOLONG, ENFILE, ENOENT, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EPERM, EROFS, ETXTBSY, EWOULDBLOCK};
#else
    static const int errors[] = {EACCES, EAGAIN, EEXIST, EILSEQ, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENOENT, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EROFS, ETXTBSY};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_open(env, err, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_open_memstream) */
static void test_p101_open_memstream(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EINVAL, ENOMEM};
#elif defined(__FreeBSD__)
    static const int errors[] = {EINVAL, ENOMEM};
#else
    static const int errors[] = {EINVAL, EMFILE, ENOMEM};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        FILE *result = p101_open_memstream(env, err, NULL, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_openat) */
static void test_p101_openat(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EACCES, EBADF, EBUSY, EDQUOT, EEXIST, EFAULT, EFBIG, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENODEV, ENOENT, ENOMEM, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EPERM, EROFS, ETXTBSY, EWOULDBLOCK};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EACCES, EAGAIN, EBADF, EEXIST, EILSEQ, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENOENT, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EROFS, ETXTBSY};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_openat(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_poll) */
static void test_p101_poll(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EFAULT, EINTR, EINVAL};
#elif defined(__FreeBSD__)
    static const int errors[] = {EFAULT, EINTR, EINVAL};
#else
    static const int errors[] = {EAGAIN, EINTR, EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_poll(env, err, NULL, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_pread) */
static void test_p101_pread(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, EDEADLK, EFAULT, EINTR, EINVAL, EIO, EISDIR, ENOBUFS, ENOMEM, ENXIO, ESPIPE, ESTALE, ETIMEDOUT};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EBUSY, ECONNRESET, EFAULT, EINTR, EINVAL, EIO, EISDIR, EOPNOTSUPP, EOVERFLOW, ESPIPE};
#else
    static const int errors[] = {EAGAIN, EBADF, EINTR, EINVAL, EIO, EISDIR, ENOBUFS, ENOMEM, ENXIO, EOVERFLOW, ESPIPE};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_pread(env, err, 0, NULL, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_pselect) */
static void test_p101_pselect(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF, EINTR, EINVAL, ENOMEM};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EBADF, EINTR, EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_pselect(env, err, 0, NULL, NULL, NULL, NULL, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_putc_unlocked) */
static void test_p101_putc_unlocked(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_putc_unlocked(env, err, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_putchar_unlocked) */
static void test_p101_putchar_unlocked(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_putchar_unlocked(env, err, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_pwrite) */
static void test_p101_pwrite(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENETDOWN, ENETUNREACH, ENOSPC, ENXIO, EPIPE, ESPIPE};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, EPIPE, ESPIPE};
#else
    static const int errors[] = {EAGAIN, EBADF, EFBIG, EINTR, EINVAL, EIO, ENOBUFS, ENOSPC, ENXIO, ESPIPE};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_pwrite(env, err, 0, NULL, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_read) */
static void test_p101_read(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EAGAIN, EBADF, EFAULT, EINTR, EINVAL, EIO, EISDIR, EWOULDBLOCK};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EFAULT, EINTR, EINVAL, EIO, EISDIR, ENOBUFS, ENOMEM, ENOTCONN, ENXIO, ESTALE, ETIMEDOUT};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EBUSY, ECONNRESET, EFAULT, EINTR, EINVAL, EIO, EISDIR, EOPNOTSUPP, EOVERFLOW};
#else
    static const int errors[] = {EAGAIN, EBADF, ECONNRESET, EINTR, EIO, EISDIR, ENOBUFS, ENOMEM, ENOTCONN, ENXIO, EOVERFLOW, ETIMEDOUT, EWOULDBLOCK};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_read(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_readv) */
static void test_p101_readv(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, EDEADLK, EFAULT, EINTR, EINVAL, EIO, EISDIR, ENOBUFS, ENOMEM, ENXIO, ESTALE, ETIMEDOUT};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EBUSY, ECONNRESET, EFAULT, EINTR, EINVAL, EIO, EISDIR, EOPNOTSUPP, EOVERFLOW};
#else
    static const int errors[] = {EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_readv(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_select) */
static void test_p101_select(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EBADF, EINTR, EINVAL, ENOMEM};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, EINTR, EINVAL};
#elif defined(__FreeBSD__)
    static const int errors[] = {EBADF, EFAULT, EINTR, EINVAL};
#else
    static const int errors[] = {EBADF, EINTR, EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_select(env, err, 0, NULL, NULL, NULL, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_setbuffer) */
static void test_p101_setbuffer(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_setbuffer(env, err, NULL, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_vdprintf) */
static void test_p101_vdprintf(struct p101_env *env, struct p101_error *err)
{
    va_list arguments;

    memset(&arguments, 0, sizeof(arguments));
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EIO};
#elif defined(__FreeBSD__)
    static const int errors[] = {EIO};
#else
    static const int errors[] = {EIO};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_vdprintf(env, err, 0, NULL, arguments);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_write) */
static void test_p101_write(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EAGAIN, EBADF, EDESTADDRREQ, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, EPERM, EPIPE, EWOULDBLOCK};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENETDOWN, ENETUNREACH, ENOSPC, ENXIO, EPIPE, EWOULDBLOCK};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, EPIPE};
#else
    static const int errors[] = {EACCES, EAGAIN, EBADF, ECONNRESET, EFBIG, EINTR, EIO, ENETDOWN, ENETUNREACH, ENOBUFS, ENOSPC, ENXIO, EPIPE, EWOULDBLOCK};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_write(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_writev) */
static void test_p101_writev(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EIO};
#elif defined(__APPLE__)
    static const int errors[] = {EAGAIN, EDESTADDRREQ, EDQUOT, EFAULT, EINVAL, ENOBUFS, EWOULDBLOCK};
#elif defined(__FreeBSD__)
    static const int errors[] = {EAGAIN, EBADF, EDESTADDRREQ, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOBUFS, ENOSPC, EPIPE};
#else
    static const int errors[] = {EINVAL};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};

        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_writev(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.errnum));
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

int main(void)
{
    struct p101_error *err;
    struct p101_env   *env;

    err = p101_error_create(false);
    if(err == NULL)
    {
        return EXIT_FAILURE;
    }
    env = p101_env_create(err, NULL);
    if(env == NULL)
    {
        p101_error_destroy(err);
        return EXIT_FAILURE;
    }
    test_p101_aio_cancel(env, err);
    test_p101_aio_error(env, err);
    test_p101_aio_fsync(env, err);
    test_p101_aio_read(env, err);
    test_p101_aio_return(env, err);
    test_p101_aio_suspend(env, err);
    test_p101_aio_write(env, err);
    test_p101_close(env, err);
    test_p101_creat(env, err);
    test_p101_dup(env, err);
    test_p101_dup2(env, err);
    test_p101_fcntl(env, err);
    test_p101_fdopen(env, err);
    test_p101_fileno(env, err);
    test_p101_fmemopen(env, err);
    test_p101_fpurge(env, err);
    test_p101_fseeko(env, err);
    test_p101_ftello(env, err);
    test_p101_getc_unlocked(env, err);
    test_p101_getchar_unlocked(env, err);
    test_p101_getdelim(env, err);
    test_p101_getline(env, err);
    test_p101_lio_listio(env, err);
    test_p101_lockf(env, err);
    test_p101_lseek(env, err);
    test_p101_open(env, err);
    test_p101_open_memstream(env, err);
    test_p101_openat(env, err);
    test_p101_poll(env, err);
    test_p101_pread(env, err);
    test_p101_pselect(env, err);
    test_p101_putc_unlocked(env, err);
    test_p101_putchar_unlocked(env, err);
    test_p101_pwrite(env, err);
    test_p101_read(env, err);
    test_p101_readv(env, err);
    test_p101_select(env, err);
    test_p101_setbuffer(env, err);
    test_p101_vdprintf(env, err);
    test_p101_write(env, err);
    test_p101_writev(env, err);
    p101_env_destroy(env);
    p101_error_destroy(err);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
