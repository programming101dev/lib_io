#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fmtmsg.h>
#include <fnmatch.h>
#include <ftw.h>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <p101_env/env.h>
#include <p101_error/error.h>
#include <p101_io/p101_aio.h>
#include <p101_io/p101_fcntl.h>
#include <p101_io/p101_poll.h>
#include <p101_io/p101_stdio.h>
#include <p101_io/p101_unistd.h>
#include <p101_io/sys/p101_select.h>
#include <p101_io/sys/p101_uio.h>
#include <pthread.h>
#include <search.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utmpx.h>

static int    failures;
static size_t fault_resource_events;
static FILE  *outcome_stream;
static bool   native_child_process;
static int    native_child_status = EXIT_SUCCESS;

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

#define P101_NATIVE_CLEANUP_ERRNO(expression)                                                                                                                                                                                                                      \
    do                                                                                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                                              \
        int p101_cleanup_status_;                                                                                                                                                                                                                                  \
                                                                                                                                                                                                                                                                   \
        p101_cleanup_status_ = (expression);                                                                                                                                                                                                                       \
        if(p101_cleanup_status_ != 0)                                                                                                                                                                                                                              \
        {                                                                                                                                                                                                                                                          \
            fprintf(stderr, "native cleanup failed: %s: %s\n", #expression, strerror(errno));                                                                                                                                                                      \
            native_passed = false;                                                                                                                                                                                                                                 \
        }                                                                                                                                                                                                                                                          \
    } while(0)

#define P101_NATIVE_CLEANUP_STATUS(expression)                                                                                                                                                                                                                     \
    do                                                                                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                                              \
        int p101_cleanup_status_ = (expression);                                                                                                                                                                                                                   \
        if(p101_cleanup_status_ != 0)                                                                                                                                                                                                                              \
        {                                                                                                                                                                                                                                                          \
            fprintf(stderr, "native cleanup failed: %s: status %d\n", #expression, p101_cleanup_status_);                                                                                                                                                          \
            native_passed = false;                                                                                                                                                                                                                                 \
        }                                                                                                                                                                                                                                                          \
    } while(0)

#define P101_NATIVE_CLEANUP_UNLINK_IF_PRESENT(path)                                                                                                                                                                                                                \
    do                                                                                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                                              \
        bool p101_cleanup_ok_;                                                                                                                                                                                                                                     \
                                                                                                                                                                                                                                                                   \
        p101_cleanup_ok_ = native_unlink_if_present(path);                                                                                                                                                                                                         \
        if(!p101_cleanup_ok_)                                                                                                                                                                                                                                      \
        {                                                                                                                                                                                                                                                          \
            native_passed = false;                                                                                                                                                                                                                                 \
        }                                                                                                                                                                                                                                                          \
    } while(0)

#define P101_NATIVE_FORMAT_PID_PATH_OR_SKIP(buffer, format)                                                                                                                                                                                                        \
    do                                                                                                                                                                                                                                                             \
    {                                                                                                                                                                                                                                                              \
        bool p101_format_ok_;                                                                                                                                                                                                                                      \
                                                                                                                                                                                                                                                                   \
        p101_format_ok_ = native_format_pid_path((buffer), sizeof(buffer), (format));                                                                                                                                                                              \
        if(!p101_format_ok_)                                                                                                                                                                                                                                       \
        {                                                                                                                                                                                                                                                          \
            fprintf(stderr, "native setup failed: path formatting\n");                                                                                                                                                                                             \
            native_child_status = 77;                                                                                                                                                                                                                              \
            goto native_child_done_;                                                                                                                                                                                                                               \
        }                                                                                                                                                                                                                                                          \
    } while(0)

struct fault_state
{
    int checks;
    int code;
};

static pid_t native_waitpid_nointr(pid_t pid, int *status) P101_ATTR_SEMANTIC_ROLE("p101:test:eintr-safe-wait-adapter")
{
    pid_t result;

    do
    {
        result = waitpid(pid, status, 0);
    } while(result < 0 && errno == EINTR);
    return result;
}

static void write_outcome(const char *wrapper, const char *domain, const char *symbol, int code, int passed)
{
    int written;

    if(outcome_stream != NULL)
    {
        written = fprintf(outcome_stream, "P101WRAPPER\t1\tFAULT\t%s\tlib_io\t%s\t%s\t%s\t%d\t%s\n", P101_TEST_PLATFORM, wrapper, domain, symbol, code, passed ? "PASS" : "FAIL");
        if(written < 0 || fflush(outcome_stream) != 0)
        {
            fprintf(stderr, "FAIL: cannot write wrapper outcome receipt\n");
            failures++;
        }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_argument_2 = open("/dev/null", O_RDONLY);
            if(native_argument_2 < 0)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_result = p101_fdopen(native_env, native_err, native_argument_2, "r");
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_fdopen: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            if(native_result != NULL)
            {
                P101_NATIVE_CLEANUP_ERRNO(fclose(native_result));
            }
            else
            {
                P101_NATIVE_CLEANUP_ERRNO(close(native_argument_2));
            }
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_fdopen: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_fdopen\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_fdopen: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
}

/* P101_TEST_CASE(p101_fileno) */
static void test_p101_fileno(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF};
    static const char *const error_names[] = {"EBADF"};
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_result = p101_fileno(native_env, native_err, native_stream);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_fileno: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_fileno: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_fileno\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_fileno: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
}

/* P101_TEST_CASE(p101_fmemopen) */
static void test_p101_fmemopen(struct p101_env *env, struct p101_error *err)
{
    unsigned char argument_2[64];
    unsigned char argument_2_before[sizeof(argument_2)];
    memset(argument_2, 0xA5, sizeof(argument_2));
    memcpy(argument_2_before, argument_2, sizeof(argument_2));
#ifdef __linux__
    static const int         errors[]      = {EINVAL, EMFILE, ENOMEM};
    static const char *const error_names[] = {"EINVAL", "EMFILE", "ENOMEM"};
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            unsigned char native_argument_2[16] = {0};
            FILE         *native_result         = p101_fmemopen(native_env, native_err, native_argument_2, 16U, "r+");
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_fmemopen: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            if(native_result != NULL)
            {
                P101_NATIVE_CLEANUP_ERRNO(fclose(native_result));
            }
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_fmemopen: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_fmemopen\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_fmemopen: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_result = p101_fpurge(native_env, native_err, native_stream);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_fpurge: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_fpurge: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_fpurge\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_fpurge: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
}

/* P101_TEST_CASE(p101_fseeko) */
static void test_p101_fseeko(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EAGAIN, EBADF, EFBIG, EINTR, EINVAL, EIO, ENOMEM, ENOSPC, ENXIO, EOVERFLOW, EPIPE, ESPIPE};
    static const char *const error_names[] = {"EAGAIN", "EBADF", "EFBIG", "EINTR", "EINVAL", "EIO", "ENOMEM", "ENOSPC", "ENXIO", "EOVERFLOW", "EPIPE", "ESPIPE"};
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_result = p101_fseeko(native_env, native_err, native_stream, 0, 0);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_fseeko: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_fseeko: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_fseeko\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_fseeko: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
}

/* P101_TEST_CASE(p101_ftello) */
static void test_p101_ftello(struct p101_env *env, struct p101_error *err)
{
#ifdef __linux__
    static const int         errors[]      = {EBADF, EOVERFLOW, ESPIPE};
    static const char *const error_names[] = {"EBADF", "EOVERFLOW", "ESPIPE"};
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            off_t native_result = p101_ftello(native_env, native_err, native_stream);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_ftello: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_ftello: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_ftello\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_ftello: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_result = p101_getc_unlocked(native_env, native_err, native_stream);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_getc_unlocked: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_getc_unlocked: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_getc_unlocked\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_getc_unlocked: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_result = p101_getchar_unlocked(native_env, native_err);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_getchar_unlocked: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_getchar_unlocked: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_getchar_unlocked\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_getchar_unlocked: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            char   *native_argument_2 = NULL;
            size_t  native_argument_3 = {0};
            ssize_t native_result     = p101_getdelim(native_env, native_err, &native_argument_2, &native_argument_3, 0, native_stream);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_getdelim: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_getdelim: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_getdelim\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_getdelim: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            char   *native_argument_2 = NULL;
            size_t  native_argument_3 = {0};
            ssize_t native_result     = p101_getline(native_env, native_err, &native_argument_2, &native_argument_3, native_stream);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_getline: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_getline: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_getline\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_getline: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    static const int         errors[]      = {EINVAL, EMFILE, ENOMEM};
    static const char *const error_names[] = {"EINVAL", "EMFILE", "ENOMEM"};
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            char  *native_argument_2 = NULL;
            size_t native_argument_3 = {0};
            FILE  *native_result     = p101_open_memstream(native_env, native_err, &native_argument_2, &native_argument_3);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_open_memstream: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_open_memstream: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_open_memstream\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_open_memstream: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_result = p101_putc_unlocked(native_env, native_err, 0, native_stream);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_putc_unlocked: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_putc_unlocked: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_putc_unlocked\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_putc_unlocked: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_result = p101_putchar_unlocked(native_env, native_err, 0);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_putchar_unlocked: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_putchar_unlocked: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_putchar_unlocked\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_putchar_unlocked: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_stream = tmpfile();
            if(native_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            char native_argument_3[PATH_MAX] = {0};
            int  native_result               = p101_setbuffer(native_env, native_err, native_stream, native_argument_3, 0);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_setbuffer: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_setbuffer: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_setbuffer\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_setbuffer: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
}

/* P101_TEST_CASE(p101_vdprintf) */
static void test_p101_vdprintf(struct p101_env *env, struct p101_error *err, ...)
{
    va_list arguments;

    va_start(arguments, err);
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
        int result = p101_vdprintf(env, err, 0, "x", arguments);
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
    {
        int   native_status = 0;
        pid_t native_pid    = fork();

        EXPECT(native_pid >= 0);
        if(native_pid == 0)
        {
            bool               native_passed = true;
            struct p101_error *native_err    = NULL;
            struct p101_env   *native_env    = NULL;

            native_child_process = true;
            failures             = 0;
            (void)alarm(2U);
            if(unsetenv("P101_CALL_LOG") != 0 || unsetenv("P101_RESOURCE_LOG") != 0)
            {
                fprintf(stderr, "native setup failed: cannot clear p101 logging environment\n");
                native_child_status = 77;
                goto native_child_done_;
            }
            native_err = p101_error_create(false);
            if(native_err == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            native_env = p101_env_create(native_err, NULL);
            if(native_env == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            FILE *native_argument_2_stream = tmpfile();
            if(native_argument_2_stream == NULL)
            {
                native_child_status = 77;
                goto native_child_done_;
            }
            int native_result = p101_vdprintf(native_env, native_err, fileno(native_argument_2_stream), "p101", arguments);
            (void)native_result;
            if(p101_error_has_error(native_err))
            {
                bool native_error_declared = false;

                for(size_t native_error_index = 0U; native_error_index < sizeof(errors) / sizeof(errors[0]); native_error_index++)
                {
                    if(p101_error_is_errno(native_err, errors[native_error_index]))
                    {
                        native_error_declared = true;
                    }
                }
                if(!native_error_declared)
                {
                    fprintf(stderr, "native smoke produced an undeclared platform failure: p101_vdprintf: %s\n", p101_error_get_message(native_err));
                    native_passed = false;
                }
                p101_error_reset(native_err);
            }
            P101_NATIVE_CLEANUP_ERRNO(fclose(native_argument_2_stream));
            native_child_status = native_passed ? EXIT_SUCCESS : EXIT_FAILURE;
        native_child_done_:
            p101_env_destroy(native_env);
            p101_error_destroy(native_err);
        }
        if(native_pid > 0)
        {
            EXPECT(native_waitpid_nointr(native_pid, &native_status) == native_pid);
            if(WIFSIGNALED(native_status))
            {
                fprintf(stderr, "native smoke terminated by signal: p101_vdprintf: %d\n", WTERMSIG(native_status));
            }
            EXPECT(WIFEXITED(native_status));
            if(WIFEXITED(native_status) && WEXITSTATUS(native_status) == 77)
            {
                fprintf(stderr, "native smoke fixture unavailable: p101_vdprintf\n");
            }
            else if(WIFEXITED(native_status))
            {
                if(WEXITSTATUS(native_status) != EXIT_SUCCESS)
                {
                    fprintf(stderr, "native smoke exited unsuccessfully: p101_vdprintf: %d\n", WEXITSTATUS(native_status));
                }
                EXPECT(WEXITSTATUS(native_status) == EXIT_SUCCESS);
            }
        }
        p101_error_reset(err);
    }
    va_end(arguments);
}

int main(void)
{
    const char        *outcome_path;
    struct p101_error *err = NULL;
    struct p101_env   *env = NULL;
    int                status;

    outcome_path = getenv("P101_WRAPPER_OUTCOME_LOG");
    if(outcome_path != NULL && outcome_path[0] != '\0')
    {
        outcome_stream = fopen(outcome_path, "a");
        if(outcome_stream == NULL)
        {
            fprintf(stderr, "FAIL: cannot open wrapper outcome receipt\n");
            failures++;
        }
    }
    if(failures == 0)
    {
        err = p101_error_create(false);
    }
    if(err != NULL)
    {
        env = p101_env_create(err, NULL);
    }
    if(env == NULL)
    {
        failures++;
    }
    else
    {
        p101_env_set_fd_observer(env, count_fd_event, NULL);
        p101_env_set_alloc_observer(env, count_alloc_event, NULL);
        p101_env_set_resource_observer(env, count_resource_event, NULL);
        if(!native_child_process)
        {
            test_p101_fdopen(env, err);
        }
        if(!native_child_process)
        {
            test_p101_fileno(env, err);
        }
        if(!native_child_process)
        {
            test_p101_fmemopen(env, err);
        }
        if(!native_child_process)
        {
            test_p101_fpurge(env, err);
        }
        if(!native_child_process)
        {
            test_p101_fseeko(env, err);
        }
        if(!native_child_process)
        {
            test_p101_ftello(env, err);
        }
        if(!native_child_process)
        {
            test_p101_getc_unlocked(env, err);
        }
        if(!native_child_process)
        {
            test_p101_getchar_unlocked(env, err);
        }
        if(!native_child_process)
        {
            test_p101_getdelim(env, err);
        }
        if(!native_child_process)
        {
            test_p101_getline(env, err);
        }
        if(!native_child_process)
        {
            test_p101_open_memstream(env, err);
        }
        if(!native_child_process)
        {
            test_p101_putc_unlocked(env, err);
        }
        if(!native_child_process)
        {
            test_p101_putchar_unlocked(env, err);
        }
        if(!native_child_process)
        {
            test_p101_setbuffer(env, err);
        }
        if(!native_child_process)
        {
            test_p101_vdprintf(env, err, 0);
        }
    }
    p101_env_destroy(env);
    p101_error_destroy(err);
    if(outcome_stream != NULL && fclose(outcome_stream) != 0)
    {
        fprintf(stderr, "FAIL: cannot close wrapper outcome receipt\n");
        failures++;
    }
    if(native_child_process)
    {
        status = native_child_status;
        if(status == EXIT_SUCCESS && failures != 0)
        {
            status = EXIT_FAILURE;
        }
    }
    else
    {
        status = failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    return status;
}
