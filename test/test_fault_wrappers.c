#include <arpa/inet.h>
#include <errno.h>
#include <fmtmsg.h>
#include <fnmatch.h>
#include <math.h>
#include <p101_env/env.h>
#include <p101_error/error.h>
#include <p101_io/io.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int    failures;
static size_t fault_resource_events;
static FILE  *outcome_stream;

#define P101_TEST_ERRNO_SENTINEL 0x5A5A

#ifdef __linux__
    #define P101_TEST_PLATFORM "linux"
#elif defined(__APPLE__)
    #define P101_TEST_PLATFORM "macos"
#elif defined(__FreeBSD__)
    #define P101_TEST_PLATFORM "freebsd"
#else
    #define P101_TEST_PLATFORM "posix"
#endif

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
    int code;
};

static void write_outcome(const char *wrapper, const char *domain, const char *symbol, int code, int passed)
{
    int written;

    if(outcome_stream == NULL)
    {
        return;
    }
    written = fprintf(outcome_stream, "P101WRAPPER\t1\tFAULT\t%s\tlib_io\t%s\t%s\t%s\t%d\t%s\n", P101_TEST_PLATFORM, wrapper, domain, symbol, code, passed ? "PASS" : "FAIL");
    if(written < 0 || fflush(outcome_stream) != 0)
    {
        fprintf(stderr, "FAIL: cannot write wrapper outcome receipt\n");
        failures++;
    }
}

static int fail_next_call(const struct p101_env *env, const char *call_name, void *user_data)
{
    struct fault_state *state;

    (void)env;
    (void)call_name;
    state = user_data;
    state->checks++;
    return state->code;
}

static void count_fd_event(const struct p101_env *env, p101_env_fd_event event, int fd, const char *file_name, const char *function_name, int line_number, void *user_data)
{
    (void)env;
    (void)event;
    (void)fd;
    (void)file_name;
    (void)function_name;
    (void)line_number;
    (void)user_data;
    fault_resource_events++;
}

static void count_alloc_event(const struct p101_env *env, p101_env_alloc_event event, const void *ptr, const void *new_ptr, size_t size, const char *file_name, const char *function_name, int line_number, void *user_data)
{
    (void)env;
    (void)event;
    (void)ptr;
    (void)new_ptr;
    (void)size;
    (void)file_name;
    (void)function_name;
    (void)line_number;
    (void)user_data;
    fault_resource_events++;
}

static void count_resource_event(const struct p101_env *env, p101_env_resource_kind event, const char *resource_class, const char *resource_id, const char *related_id, size_t size, const char *metadata, const char *file_name, const char *function_name,
                                 int line_number, void *user_data)
{
    (void)env;
    (void)event;
    (void)resource_class;
    (void)resource_id;
    (void)related_id;
    (void)size;
    (void)metadata;
    (void)file_name;
    (void)function_name;
    (void)line_number;
    (void)user_data;
    fault_resource_events++;
}

/* P101_TEST_CASE(p101_aio_cancel) */
static void test_p101_aio_cancel(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF, ENOSYS};
    static const char *const error_names[] = {"EBADF", "ENOSYS"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
#else
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_cancel(env, err, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_aio_cancel", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_error) */
static void test_p101_aio_error(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EINVAL, ENOSYS};
    static const char *const error_names[] = {"EINVAL", "ENOSYS"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#else
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_error(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == state.code);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_aio_error", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_fsync) */
static void test_p101_aio_fsync(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EAGAIN, EBADF, EINVAL, ENOSYS};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EINVAL", "ENOSYS"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, EINVAL};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EINVAL"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EINVAL, EOPNOTSUPP};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EINVAL", "EOPNOTSUPP"};
#else
    static const int         errors[]      = {EAGAIN, EBADF, EINVAL};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_fsync(env, err, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_aio_fsync", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_read) */
static void test_p101_aio_read(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EAGAIN, EBADF, EINVAL, ENOSYS, EOVERFLOW};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EINVAL", "ENOSYS", "EOVERFLOW"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, ECANCELED, EFAULT, EINVAL, ENOSYS, EOVERFLOW};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECANCELED", "EFAULT", "EINVAL", "ENOSYS", "EOVERFLOW"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, ECANCELED, EFAULT, EINVAL, EOPNOTSUPP, EOVERFLOW};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECANCELED", "EFAULT", "EINVAL", "EOPNOTSUPP", "EOVERFLOW"};
#else
    static const int         errors[]      = {EAGAIN, EBADF, ECANCELED, EINVAL, EOVERFLOW};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECANCELED", "EINVAL", "EOVERFLOW"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_read(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_aio_read", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_return) */
static void test_p101_aio_return(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EINVAL, ENOSYS};
    static const char *const error_names[] = {"EINVAL", "ENOSYS"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EINPROGRESS, EINVAL};
    static const char *const error_names[] = {"EINPROGRESS", "EINVAL"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#else
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_aio_return(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == ((ssize_t)-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_aio_return", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_suspend) */
static void test_p101_aio_suspend(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EAGAIN, EINTR, ENOSYS};
    static const char *const error_names[] = {"EAGAIN", "EINTR", "ENOSYS"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EINTR, EINVAL};
    static const char *const error_names[] = {"EAGAIN", "EINTR", "EINVAL"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EINTR, EINVAL};
    static const char *const error_names[] = {"EAGAIN", "EINTR", "EINVAL"};
#else
    static const int         errors[]      = {EAGAIN, EINTR};
    static const char *const error_names[] = {"EAGAIN", "EINTR"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_suspend(env, err, NULL, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_aio_suspend", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_aio_write) */
static void test_p101_aio_write(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EAGAIN, EBADF, EFBIG, EINVAL, ENOSYS};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EFBIG", "EINVAL", "ENOSYS"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, ECANCELED, EFAULT, EINVAL, ENOSYS};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECANCELED", "EFAULT", "EINVAL", "ENOSYS"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, ECANCELED, EFAULT, EINVAL, EOPNOTSUPP};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECANCELED", "EFAULT", "EINVAL", "EOPNOTSUPP"};
#else
    static const int         errors[]      = {EAGAIN, EBADF, ECANCELED, EFBIG, EINVAL};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECANCELED", "EFBIG", "EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_aio_write(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_aio_write", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_close) */
static void test_p101_close(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF, EINTR, EIO, ENOSPC};
    static const char *const error_names[] = {"EBADF", "EINTR", "EIO", "ENOSPC"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EBADF, EINTR, EIO};
    static const char *const error_names[] = {"EBADF", "EINTR", "EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EBADF, EINTR, ENOSPC};
    static const char *const error_names[] = {"EBADF", "EINTR", "ENOSPC"};
#else
    static const int         errors[]      = {EBADF, EINPROGRESS, EINTR, EIO};
    static const char *const error_names[] = {"EBADF", "EINPROGRESS", "EINTR", "EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_close(env, err, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_close", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_creat) */
static void test_p101_creat(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EACCES, EBUSY, EDQUOT, EEXIST, EFAULT, EFBIG, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENODEV, ENOENT, ENOMEM, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EPERM, EROFS, ETXTBSY, EWOULDBLOCK};
    static const char *const error_names[] = {"EACCES", "EBUSY",  "EDQUOT", "EEXIST", "EFAULT",  "EFBIG", "EINTR",      "EINVAL",    "EISDIR", "ELOOP", "EMFILE",  "ENAMETOOLONG", "ENFILE",
                                              "ENODEV", "ENOENT", "ENOMEM", "ENOSPC", "ENOTDIR", "ENXIO", "EOPNOTSUPP", "EOVERFLOW", "EPERM",  "EROFS", "ETXTBSY", "EWOULDBLOCK"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_creat(env, err, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_creat", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_dup) */
static void test_p101_dup(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF, EBUSY, EMFILE, ENOMEM};
    static const char *const error_names[] = {"EBADF", "EBUSY", "EMFILE", "ENOMEM"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EBADF, EINTR, EMFILE};
    static const char *const error_names[] = {"EBADF", "EINTR", "EMFILE"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EBADF, EMFILE};
    static const char *const error_names[] = {"EBADF", "EMFILE"};
#else
    static const int         errors[]      = {EBADF, EMFILE};
    static const char *const error_names[] = {"EBADF", "EMFILE"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_dup(env, err, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_dup", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_dup2) */
static void test_p101_dup2(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF, EBUSY, EINTR, EMFILE, ENOMEM};
    static const char *const error_names[] = {"EBADF", "EBUSY", "EINTR", "EMFILE", "ENOMEM"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EBADF, EINTR, EMFILE};
    static const char *const error_names[] = {"EBADF", "EINTR", "EMFILE"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
#else
    static const int         errors[]      = {EBADF, EINTR, EIO};
    static const char *const error_names[] = {"EBADF", "EINTR", "EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_dup2(env, err, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_dup2", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fcntl) */
static void test_p101_fcntl(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EINVAL};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "EINVAL"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EDEADLK, EFBIG, EINTR, EINVAL, EMFILE, ENOLCK, ENOSPC, ENOTSUP, EOVERFLOW, EPERM, ESRCH, EXDEV};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "EDEADLK", "EFBIG", "EINTR", "EINVAL", "EMFILE", "ENOLCK", "ENOSPC", "ENOTSUP", "EOVERFLOW", "EPERM", "ESRCH", "EXDEV"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EBUSY, EDEADLK, EINTR, EINVAL, EMFILE, ENOLCK, ENOTTY, EOPNOTSUPP, EOVERFLOW, EPERM, ESRCH};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EBUSY", "EDEADLK", "EINTR", "EINVAL", "EMFILE", "ENOLCK", "ENOTTY", "EOPNOTSUPP", "EOVERFLOW", "EPERM", "ESRCH"};
#else
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, EMFILE, ENOLCK, EOVERFLOW, EPERM, ESRCH};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "EDEADLK", "EINTR", "EINVAL", "EMFILE", "ENOLCK", "EOVERFLOW", "EPERM", "ESRCH"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_fcntl(env, err, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_fcntl", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fdopen) */
static void test_p101_fdopen(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EINVAL, ENOMEM};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "EINVAL", "ENOMEM"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EDEADLK, EFBIG, EINTR, EINVAL, EMFILE, ENOLCK, ENOSPC, ENOTSUP, EOVERFLOW, EPERM, ESRCH, EXDEV};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "EDEADLK", "EFBIG", "EINTR", "EINVAL", "EMFILE", "ENOLCK", "ENOSPC", "ENOTSUP", "EOVERFLOW", "EPERM", "ESRCH", "EXDEV"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EBUSY, EDEADLK, EINTR, EINVAL, EMFILE, ENOLCK, ENOTTY, EOPNOTSUPP, EOVERFLOW, EPERM, ESRCH};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EBUSY", "EDEADLK", "EINTR", "EINVAL", "EMFILE", "ENOLCK", "ENOTTY", "EOPNOTSUPP", "EOVERFLOW", "EPERM", "ESRCH"};
#else
    static const int         errors[]      = {EBADF, EINVAL, EMFILE, ENOMEM};
    static const char *const error_names[] = {"EBADF", "EINVAL", "EMFILE", "ENOMEM"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        FILE *result = p101_fdopen(env, err, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (NULL));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_fdopen", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fileno) */
static void test_p101_fileno(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
#else
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_fileno(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_fileno", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fmemopen) */
static void test_p101_fmemopen(struct p101_env *env, struct p101_error *err)
{
    unsigned char argument_2[64];
    unsigned char argument_2_before[sizeof(argument_2)];
    memset(argument_2, 0xA5, sizeof(argument_2));
    memcpy(argument_2_before, argument_2, sizeof(argument_2));
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#else
    static const int         errors[]      = {EINVAL, EMFILE, ENOMEM};
    static const char *const error_names[] = {"EINVAL", "EMFILE", "ENOMEM"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        FILE *result = p101_fmemopen(env, err, argument_2, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (NULL));
        EXPECT(memcmp(argument_2, argument_2_before, sizeof(argument_2)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_fmemopen", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fpurge) */
static void test_p101_fpurge(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_fpurge(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_fpurge", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_fseeko) */
static void test_p101_fseeko(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENETDOWN, ENETUNREACH, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE, EWOULDBLOCK};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECONNRESET", "EDEADLK", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENETDOWN", "ENETUNREACH", "ENOSPC", "ENXIO", "EOVERFLOW", "EPIPE", "ESPIPE", "EWOULDBLOCK"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOSPC", "ENXIO", "EOVERFLOW", "EPIPE", "ESPIPE"};
#else
    static const int         errors[]      = {EAGAIN, EBADF, EFBIG, EINTR, EINVAL, EIO, ENOMEM, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOMEM", "ENOSPC", "ENXIO", "EOVERFLOW", "EPIPE", "ESPIPE"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_fseeko(env, err, NULL, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_fseeko", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_ftello) */
static void test_p101_ftello(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENETDOWN, ENETUNREACH, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE, EWOULDBLOCK};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECONNRESET", "EDEADLK", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENETDOWN", "ENETUNREACH", "ENOSPC", "ENXIO", "EOVERFLOW", "EPIPE", "ESPIPE", "EWOULDBLOCK"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOSPC", "ENXIO", "EOVERFLOW", "EPIPE", "ESPIPE"};
#else
    static const int         errors[]      = {EBADF, EOVERFLOW, ESPIPE};
    static const char *const error_names[] = {"EBADF", "EOVERFLOW", "ESPIPE"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        off_t result = p101_ftello(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == ((off_t)-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_ftello", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_getc_unlocked) */
static void test_p101_getc_unlocked(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_getc_unlocked(env, err, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_getc_unlocked", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_getchar_unlocked) */
static void test_p101_getchar_unlocked(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_getchar_unlocked(env, err);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_getchar_unlocked", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_getdelim) */
static void test_p101_getdelim(struct p101_env *env, struct p101_error *err)
{
    char         *argument_2[4];
    unsigned char argument_2_before[sizeof(argument_2)];
    memset(argument_2, 0xA5, sizeof(argument_2));
    memcpy(argument_2_before, argument_2, sizeof(argument_2));
    size_t        argument_3[4];
    unsigned char argument_3_before[sizeof(argument_3)];
    memset(argument_3, 0xA5, sizeof(argument_3));
    memcpy(argument_3_before, argument_3, sizeof(argument_3));
#ifdef __linux__
    static const int         errors[]      = {EINVAL, ENOMEM};
    static const char *const error_names[] = {"EINVAL", "ENOMEM"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EINVAL, EOVERFLOW};
    static const char *const error_names[] = {"EINVAL", "EOVERFLOW"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EINVAL, EOVERFLOW};
    static const char *const error_names[] = {"EINVAL", "EOVERFLOW"};
#else
    static const int         errors[]      = {EINVAL, ENOMEM, EOVERFLOW};
    static const char *const error_names[] = {"EINVAL", "ENOMEM", "EOVERFLOW"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_getdelim(env, err, argument_2, argument_3, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == ((ssize_t)-1));
        EXPECT(memcmp(argument_2, argument_2_before, sizeof(argument_2)) == 0);
        EXPECT(memcmp(argument_3, argument_3_before, sizeof(argument_3)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_getdelim", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_getline) */
static void test_p101_getline(struct p101_env *env, struct p101_error *err)
{
    char         *argument_2[4];
    unsigned char argument_2_before[sizeof(argument_2)];
    memset(argument_2, 0xA5, sizeof(argument_2));
    memcpy(argument_2_before, argument_2, sizeof(argument_2));
    size_t        argument_3[4];
    unsigned char argument_3_before[sizeof(argument_3)];
    memset(argument_3, 0xA5, sizeof(argument_3));
    memcpy(argument_3_before, argument_3, sizeof(argument_3));
#ifdef __linux__
    static const int         errors[]      = {EINVAL, ENOMEM};
    static const char *const error_names[] = {"EINVAL", "ENOMEM"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EINVAL, EOVERFLOW};
    static const char *const error_names[] = {"EINVAL", "EOVERFLOW"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EINVAL, EOVERFLOW};
    static const char *const error_names[] = {"EINVAL", "EOVERFLOW"};
#else
    static const int         errors[]      = {EINVAL, ENOMEM, EOVERFLOW};
    static const char *const error_names[] = {"EINVAL", "ENOMEM", "EOVERFLOW"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_getline(env, err, argument_2, argument_3, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == ((ssize_t)-1));
        EXPECT(memcmp(argument_2, argument_2_before, sizeof(argument_2)) == 0);
        EXPECT(memcmp(argument_3, argument_3_before, sizeof(argument_3)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_getline", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_lio_listio) */
static void test_p101_lio_listio(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EAGAIN, EINTR, EINVAL, EIO};
    static const char *const error_names[] = {"EAGAIN", "EINTR", "EINVAL", "EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EINTR, EINVAL, EIO};
    static const char *const error_names[] = {"EAGAIN", "EINTR", "EINVAL", "EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EINTR, EINVAL, EIO};
    static const char *const error_names[] = {"EAGAIN", "EINTR", "EINVAL", "EIO"};
#else
    static const int         errors[]      = {EAGAIN, EINPROGRESS, EINTR, EINVAL, EIO};
    static const char *const error_names[] = {"EAGAIN", "EINPROGRESS", "EINTR", "EINVAL", "EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_lio_listio(env, err, 0, NULL, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_lio_listio", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_lockf) */
static void test_p101_lockf(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, ENOLCK};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "EDEADLK", "EINTR", "EINVAL", "ENOLCK"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, ENOLCK, EOPNOTSUPP};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDEADLK", "EINTR", "EINVAL", "ENOLCK", "EOPNOTSUPP"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, ENOLCK};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDEADLK", "EINTR", "EINVAL", "ENOLCK"};
#else
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EDEADLK, EINTR, EINVAL, ENOLCK, EOPNOTSUPP, EOVERFLOW};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "EDEADLK", "EINTR", "EINVAL", "ENOLCK", "EOPNOTSUPP", "EOVERFLOW"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_lockf(env, err, 0, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_lockf", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_lseek) */
static void test_p101_lseek(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF, EINVAL, ENXIO, EOVERFLOW, ESPIPE};
    static const char *const error_names[] = {"EBADF", "EINVAL", "ENXIO", "EOVERFLOW", "ESPIPE"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EBADF, EINVAL, ENXIO, EOVERFLOW, ESPIPE};
    static const char *const error_names[] = {"EBADF", "EINVAL", "ENXIO", "EOVERFLOW", "ESPIPE"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EBADF, EINVAL, ENXIO, EOVERFLOW, ESPIPE};
    static const char *const error_names[] = {"EBADF", "EINVAL", "ENXIO", "EOVERFLOW", "ESPIPE"};
#else
    static const int         errors[]      = {EBADF, EINVAL, ENXIO, EOVERFLOW, ESPIPE};
    static const char *const error_names[] = {"EBADF", "EINVAL", "ENXIO", "EOVERFLOW", "ESPIPE"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        off_t result = p101_lseek(env, err, 0, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == ((off_t)-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_lseek", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_open) */
static void test_p101_open(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EACCES, EBUSY, EDQUOT, EEXIST, EFAULT, EFBIG, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENODEV, ENOENT, ENOMEM, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EPERM, EROFS, ETXTBSY, EWOULDBLOCK};
    static const char *const error_names[] = {"EACCES", "EBUSY",  "EDQUOT", "EEXIST", "EFAULT",  "EFBIG", "EINTR",      "EINVAL",    "EISDIR", "ELOOP", "EMFILE",  "ENAMETOOLONG", "ENFILE",
                                              "ENODEV", "ENOENT", "ENOMEM", "ENOSPC", "ENOTDIR", "ENXIO", "EOPNOTSUPP", "EOVERFLOW", "EPERM",  "EROFS", "ETXTBSY", "EWOULDBLOCK"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EDEADLK, EDQUOT, EEXIST, EFAULT, EILSEQ, EINTR, EINVAL, EIO, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENOENT, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EROFS, ETXTBSY, EWOULDBLOCK};
    static const char *const error_names[] = {"EACCES", "EAGAIN",       "EBADF",  "EDEADLK", "EDQUOT", "EEXIST",  "EFAULT", "EILSEQ",     "EINTR",     "EINVAL", "EIO",     "EISDIR",     "ELOOP",
                                              "EMFILE", "ENAMETOOLONG", "ENFILE", "ENOENT",  "ENOSPC", "ENOTDIR", "ENXIO",  "EOPNOTSUPP", "EOVERFLOW", "EROFS",  "ETXTBSY", "EWOULDBLOCK"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EACCES, EBADF, EDQUOT, EEXIST, EFAULT, EINTR, EINVAL, EIO, EISDIR, ELOOP, EMFILE, EMLINK, ENAMETOOLONG, ENFILE, ENOENT, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EPERM, EROFS, ETXTBSY, EWOULDBLOCK};
    static const char *const error_names[] = {"EACCES",       "EBADF",  "EDQUOT", "EEXIST", "EFAULT",  "EINTR", "EINVAL",     "EIO",   "EISDIR", "ELOOP",   "EMFILE",     "EMLINK",
                                              "ENAMETOOLONG", "ENFILE", "ENOENT", "ENOSPC", "ENOTDIR", "ENXIO", "EOPNOTSUPP", "EPERM", "EROFS",  "ETXTBSY", "EWOULDBLOCK"};
#else
    static const int         errors[]      = {EACCES, EAGAIN, EEXIST, EILSEQ, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENOENT, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EROFS, ETXTBSY};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EEXIST", "EILSEQ", "EINTR", "EINVAL", "EISDIR", "ELOOP", "EMFILE", "ENAMETOOLONG", "ENFILE", "ENOENT", "ENOSPC", "ENOTDIR", "ENXIO", "EOPNOTSUPP", "EOVERFLOW", "EROFS", "ETXTBSY"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_open(env, err, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_open", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_open_memstream) */
static void test_p101_open_memstream(struct p101_env *env, struct p101_error *err)
{
    char         *argument_2[4];
    unsigned char argument_2_before[sizeof(argument_2)];
    memset(argument_2, 0xA5, sizeof(argument_2));
    memcpy(argument_2_before, argument_2, sizeof(argument_2));
    size_t        argument_3[4];
    unsigned char argument_3_before[sizeof(argument_3)];
    memset(argument_3, 0xA5, sizeof(argument_3));
    memcpy(argument_3_before, argument_3, sizeof(argument_3));
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EINVAL, ENOMEM};
    static const char *const error_names[] = {"EINVAL", "ENOMEM"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EINVAL, ENOMEM};
    static const char *const error_names[] = {"EINVAL", "ENOMEM"};
#else
    static const int         errors[]      = {EINVAL, EMFILE, ENOMEM};
    static const char *const error_names[] = {"EINVAL", "EMFILE", "ENOMEM"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        FILE *result = p101_open_memstream(env, err, argument_2, argument_3);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (NULL));
        EXPECT(memcmp(argument_2, argument_2_before, sizeof(argument_2)) == 0);
        EXPECT(memcmp(argument_3, argument_3_before, sizeof(argument_3)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_open_memstream", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_openat) */
static void test_p101_openat(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int errors[] = {EACCES, EBADF, EBUSY, EDQUOT, EEXIST, EFAULT, EFBIG, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENODEV, ENOENT, ENOMEM, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EPERM, EROFS, ETXTBSY, EWOULDBLOCK};
    static const char *const error_names[] = {"EACCES", "EBADF",  "EBUSY",  "EDQUOT", "EEXIST", "EFAULT",  "EFBIG", "EINTR",      "EINVAL",    "EISDIR", "ELOOP", "EMFILE",  "ENAMETOOLONG",
                                              "ENFILE", "ENODEV", "ENOENT", "ENOMEM", "ENOSPC", "ENOTDIR", "ENXIO", "EOPNOTSUPP", "EOVERFLOW", "EPERM",  "EROFS", "ETXTBSY", "EWOULDBLOCK"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, EEXIST, EILSEQ, EINTR, EINVAL, EISDIR, ELOOP, EMFILE, ENAMETOOLONG, ENFILE, ENOENT, ENOSPC, ENOTDIR, ENXIO, EOPNOTSUPP, EOVERFLOW, EROFS, ETXTBSY};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "EEXIST", "EILSEQ", "EINTR", "EINVAL", "EISDIR", "ELOOP", "EMFILE", "ENAMETOOLONG", "ENFILE", "ENOENT", "ENOSPC", "ENOTDIR", "ENXIO", "EOPNOTSUPP", "EOVERFLOW", "EROFS", "ETXTBSY"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_openat(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_openat", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_poll) */
static void test_p101_poll(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EFAULT, EINTR, EINVAL};
    static const char *const error_names[] = {"EAGAIN", "EFAULT", "EINTR", "EINVAL"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EFAULT, EINTR, EINVAL};
    static const char *const error_names[] = {"EFAULT", "EINTR", "EINVAL"};
#else
    static const int         errors[]      = {EAGAIN, EINTR, EINVAL};
    static const char *const error_names[] = {"EAGAIN", "EINTR", "EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_poll(env, err, NULL, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_poll", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_pread) */
static void test_p101_pread(struct p101_env *env, struct p101_error *err)
{
    unsigned char argument_3[64];
    unsigned char argument_3_before[sizeof(argument_3)];
    memset(argument_3, 0xA5, sizeof(argument_3));
    memcpy(argument_3_before, argument_3, sizeof(argument_3));
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, EDEADLK, EFAULT, EINTR, EINVAL, EIO, EISDIR, ENOBUFS, ENOMEM, ENXIO, ESPIPE, ESTALE, ETIMEDOUT};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDEADLK", "EFAULT", "EINTR", "EINVAL", "EIO", "EISDIR", "ENOBUFS", "ENOMEM", "ENXIO", "ESPIPE", "ESTALE", "ETIMEDOUT"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EBUSY, ECONNRESET, EFAULT, EINTR, EINVAL, EIO, EISDIR, EOPNOTSUPP, EOVERFLOW, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EBUSY", "ECONNRESET", "EFAULT", "EINTR", "EINVAL", "EIO", "EISDIR", "EOPNOTSUPP", "EOVERFLOW", "ESPIPE"};
#else
    static const int         errors[]      = {EAGAIN, EBADF, EINTR, EINVAL, EIO, EISDIR, ENOBUFS, ENOMEM, ENXIO, EOVERFLOW, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EINTR", "EINVAL", "EIO", "EISDIR", "ENOBUFS", "ENOMEM", "ENXIO", "EOVERFLOW", "ESPIPE"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_pread(env, err, 0, argument_3, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(memcmp(argument_3, argument_3_before, sizeof(argument_3)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_pread", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_pselect) */
static void test_p101_pselect(struct p101_env *env, struct p101_error *err)
{
    fd_set        argument_3[4];
    unsigned char argument_3_before[sizeof(argument_3)];
    memset(argument_3, 0xA5, sizeof(argument_3));
    memcpy(argument_3_before, argument_3, sizeof(argument_3));
    fd_set        argument_4[4];
    unsigned char argument_4_before[sizeof(argument_4)];
    memset(argument_4, 0xA5, sizeof(argument_4));
    memcpy(argument_4_before, argument_4, sizeof(argument_4));
    fd_set        argument_5[4];
    unsigned char argument_5_before[sizeof(argument_5)];
    memset(argument_5, 0xA5, sizeof(argument_5));
    memcpy(argument_5_before, argument_5, sizeof(argument_5));
#ifdef __linux__
    static const int         errors[]      = {EBADF, EINTR, EINVAL, ENOMEM};
    static const char *const error_names[] = {"EBADF", "EINTR", "EINVAL", "ENOMEM"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EBADF, EINTR, EINVAL};
    static const char *const error_names[] = {"EBADF", "EINTR", "EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_pselect(env, err, 0, argument_3, argument_4, argument_5, NULL, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(memcmp(argument_3, argument_3_before, sizeof(argument_3)) == 0);
        EXPECT(memcmp(argument_4, argument_4_before, sizeof(argument_4)) == 0);
        EXPECT(memcmp(argument_5, argument_5_before, sizeof(argument_5)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_pselect", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_putc_unlocked) */
static void test_p101_putc_unlocked(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_putc_unlocked(env, err, 0, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_putc_unlocked", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_putchar_unlocked) */
static void test_p101_putchar_unlocked(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_putchar_unlocked(env, err, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_putchar_unlocked", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_pwrite) */
static void test_p101_pwrite(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENETDOWN, ENETUNREACH, ENOSPC, ENXIO, EPIPE, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECONNRESET", "EDEADLK", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENETDOWN", "ENETUNREACH", "ENOSPC", "ENXIO", "EPIPE", "ESPIPE"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, EPIPE, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOSPC", "EPIPE", "ESPIPE"};
#else
    static const int         errors[]      = {EAGAIN, EBADF, EFBIG, EINTR, EINVAL, EIO, ENOBUFS, ENOSPC, ENXIO, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOBUFS", "ENOSPC", "ENXIO", "ESPIPE"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_pwrite(env, err, 0, NULL, 0, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_pwrite", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_read) */
static void test_p101_read(struct p101_env *env, struct p101_error *err)
{
    unsigned char argument_3[64];
    unsigned char argument_3_before[sizeof(argument_3)];
    memset(argument_3, 0xA5, sizeof(argument_3));
    memcpy(argument_3_before, argument_3, sizeof(argument_3));
#ifdef __linux__
    static const int         errors[]      = {EAGAIN, EBADF, EFAULT, EINTR, EINVAL, EIO, EISDIR, EWOULDBLOCK};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EFAULT", "EINTR", "EINVAL", "EIO", "EISDIR", "EWOULDBLOCK"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EFAULT, EINTR, EINVAL, EIO, EISDIR, ENOBUFS, ENOMEM, ENOTCONN, ENXIO, ESTALE, ETIMEDOUT};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECONNRESET", "EDEADLK", "EFAULT", "EINTR", "EINVAL", "EIO", "EISDIR", "ENOBUFS", "ENOMEM", "ENOTCONN", "ENXIO", "ESTALE", "ETIMEDOUT"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EBUSY, ECONNRESET, EFAULT, EINTR, EINVAL, EIO, EISDIR, EOPNOTSUPP, EOVERFLOW};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EBUSY", "ECONNRESET", "EFAULT", "EINTR", "EINVAL", "EIO", "EISDIR", "EOPNOTSUPP", "EOVERFLOW"};
#else
    static const int         errors[]      = {EAGAIN, EBADF, ECONNRESET, EINTR, EIO, EISDIR, ENOBUFS, ENOMEM, ENOTCONN, ENXIO, EOVERFLOW, ETIMEDOUT, EWOULDBLOCK};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECONNRESET", "EINTR", "EIO", "EISDIR", "ENOBUFS", "ENOMEM", "ENOTCONN", "ENXIO", "EOVERFLOW", "ETIMEDOUT", "EWOULDBLOCK"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_read(env, err, 0, argument_3, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(memcmp(argument_3, argument_3_before, sizeof(argument_3)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_read", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_readv) */
static void test_p101_readv(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, EDEADLK, EFAULT, EINTR, EINVAL, EIO, EISDIR, ENOBUFS, ENOMEM, ENXIO, ESTALE, ETIMEDOUT};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDEADLK", "EFAULT", "EINTR", "EINVAL", "EIO", "EISDIR", "ENOBUFS", "ENOMEM", "ENXIO", "ESTALE", "ETIMEDOUT"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EBUSY, ECONNRESET, EFAULT, EINTR, EINVAL, EIO, EISDIR, EOPNOTSUPP, EOVERFLOW};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EBUSY", "ECONNRESET", "EFAULT", "EINTR", "EINVAL", "EIO", "EISDIR", "EOPNOTSUPP", "EOVERFLOW"};
#else
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_readv(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_readv", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_select) */
static void test_p101_select(struct p101_env *env, struct p101_error *err)
{
    fd_set        argument_3[4];
    unsigned char argument_3_before[sizeof(argument_3)];
    memset(argument_3, 0xA5, sizeof(argument_3));
    memcpy(argument_3_before, argument_3, sizeof(argument_3));
    fd_set        argument_4[4];
    unsigned char argument_4_before[sizeof(argument_4)];
    memset(argument_4, 0xA5, sizeof(argument_4));
    memcpy(argument_4_before, argument_4, sizeof(argument_4));
    fd_set        argument_5[4];
    unsigned char argument_5_before[sizeof(argument_5)];
    memset(argument_5, 0xA5, sizeof(argument_5));
    memcpy(argument_5_before, argument_5, sizeof(argument_5));
#ifdef __linux__
    static const int         errors[]      = {EBADF, EINTR, EINVAL, ENOMEM};
    static const char *const error_names[] = {"EBADF", "EINTR", "EINVAL", "ENOMEM"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, EINTR, EINVAL};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EINTR", "EINVAL"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EBADF, EFAULT, EINTR, EINVAL};
    static const char *const error_names[] = {"EBADF", "EFAULT", "EINTR", "EINVAL"};
#else
    static const int         errors[]      = {EBADF, EINTR, EINVAL};
    static const char *const error_names[] = {"EBADF", "EINTR", "EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_select(env, err, 0, argument_3, argument_4, argument_5, NULL);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(memcmp(argument_3, argument_3_before, sizeof(argument_3)) == 0);
        EXPECT(memcmp(argument_4, argument_4_before, sizeof(argument_4)) == 0);
        EXPECT(memcmp(argument_5, argument_5_before, sizeof(argument_5)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_select", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_setbuffer) */
static void test_p101_setbuffer(struct p101_env *env, struct p101_error *err)
{
    char          argument_3[4];
    unsigned char argument_3_before[sizeof(argument_3)];
    memset(argument_3, 0xA5, sizeof(argument_3));
    memcpy(argument_3_before, argument_3, sizeof(argument_3));
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_setbuffer(env, err, NULL, argument_3, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(memcmp(argument_3, argument_3_before, sizeof(argument_3)) == 0);
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_setbuffer", "errno", error_names[index], state.code, failures == failures_before);
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
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#else
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        int result = p101_vdprintf(env, err, 0, "p101", arguments);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_vdprintf", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_write) */
static void test_p101_write(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EAGAIN, EBADF, EDESTADDRREQ, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, EPERM, EPIPE, EWOULDBLOCK};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDESTADDRREQ", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOSPC", "EPERM", "EPIPE", "EWOULDBLOCK"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EBADF, ECONNRESET, EDEADLK, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENETDOWN, ENETUNREACH, ENOSPC, ENXIO, EPIPE, EWOULDBLOCK};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "ECONNRESET", "EDEADLK", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENETDOWN", "ENETUNREACH", "ENOSPC", "ENXIO", "EPIPE", "EWOULDBLOCK"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOSPC, EPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOSPC", "EPIPE"};
#else
    static const int         errors[]      = {EACCES, EAGAIN, EBADF, ECONNRESET, EFBIG, EINTR, EIO, ENETDOWN, ENETUNREACH, ENOBUFS, ENOSPC, ENXIO, EPIPE, EWOULDBLOCK};
    static const char *const error_names[] = {"EACCES", "EAGAIN", "EBADF", "ECONNRESET", "EFBIG", "EINTR", "EIO", "ENETDOWN", "ENETUNREACH", "ENOBUFS", "ENOSPC", "ENXIO", "EPIPE", "EWOULDBLOCK"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_write(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_write", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

/* P101_TEST_CASE(p101_writev) */
static void test_p101_writev(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EIO};
    static const char *const error_names[] = {"EIO"};
#elif defined(__APPLE__)
    static const int         errors[]      = {EAGAIN, EDESTADDRREQ, EDQUOT, EFAULT, EINVAL, ENOBUFS, EWOULDBLOCK};
    static const char *const error_names[] = {"EAGAIN", "EDESTADDRREQ", "EDQUOT", "EFAULT", "EINVAL", "ENOBUFS", "EWOULDBLOCK"};
#elif defined(__FreeBSD__)
    static const int         errors[]      = {EAGAIN, EBADF, EDESTADDRREQ, EDQUOT, EFAULT, EFBIG, EINTR, EINVAL, EIO, ENOBUFS, ENOSPC, EPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EDESTADDRREQ", "EDQUOT", "EFAULT", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOBUFS", "ENOSPC", "EPIPE"};
#else
    static const int         errors[]      = {EINVAL};
    static const char *const error_names[] = {"EINVAL"};
#endif

    for(size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); index++)
    {
        struct fault_state state = {0, errors[index]};
        int                failures_before;

        failures_before = failures;
        EXPECT(p101_error_has_no_error(err));
        fault_resource_events = 0U;
        errno                 = P101_TEST_ERRNO_SENTINEL;
        p101_env_set_fault_injector(env, fail_next_call, &state);
        ssize_t result = p101_writev(env, err, 0, NULL, 0);
        (void)result;
        EXPECT(state.checks == 1);
        EXPECT(p101_error_is_errno(err, state.code));
        EXPECT(errno == P101_TEST_ERRNO_SENTINEL);
        EXPECT(result == (-1));
        EXPECT(fault_resource_events == 0U);
        write_outcome("p101_writev", "errno", error_names[index], state.code, failures == failures_before);
        p101_error_reset(err);
    }
    p101_env_set_fault_injector(env, NULL, NULL);
}

int main(void)
{
    const char        *outcome_path;
    struct p101_error *err;
    struct p101_env   *env;

    outcome_path = getenv("P101_WRAPPER_OUTCOME_LOG");
    if(outcome_path != NULL && outcome_path[0] != '\0')
    {
        outcome_stream = fopen(outcome_path, "a");
        if(outcome_stream == NULL)
        {
            fprintf(stderr, "FAIL: cannot open wrapper outcome receipt\n");
            return EXIT_FAILURE;
        }
    }
    err = p101_error_create(false);
    if(err == NULL)
    {
        if(outcome_stream != NULL)
        {
            (void)fclose(outcome_stream);
        }
        return EXIT_FAILURE;
    }
    env = p101_env_create(err, NULL);
    if(env == NULL)
    {
        p101_error_destroy(err);
        if(outcome_stream != NULL)
        {
            (void)fclose(outcome_stream);
        }
        return EXIT_FAILURE;
    }
    p101_env_set_fd_observer(env, count_fd_event, NULL);
    p101_env_set_alloc_observer(env, count_alloc_event, NULL);
    p101_env_set_resource_observer(env, count_resource_event, NULL);
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
    if(outcome_stream != NULL && fclose(outcome_stream) != 0)
    {
        fprintf(stderr, "FAIL: cannot close wrapper outcome receipt\n");
        failures++;
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
