#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <p101_env/env.h>
#include <p101_error/error.h>
#include <p101_io/p101_aio.h>
#include <p101_io/p101_fcntl.h>
#include <p101_io/p101_poll.h>
#include <p101_io/p101_stdio.h>
#include <p101_io/p101_unistd.h>
#include <p101_io/sys/p101_select.h>
#include <p101_io/sys/p101_uio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static int call_vdprintf(const struct p101_env *env, struct p101_error *err, int fd, const char *format, ...)
{
    va_list arguments;
    int     result;

    va_start(arguments, format);
    /* P101_TEST_CASE(p101_vdprintf) */
    result = p101_vdprintf(env, err, fd, format, arguments);
    va_end(arguments);
    return result;
}

static void test_file_locking(const struct p101_env *env)
{
    FILE *stream;

    stream = tmpfile();
    EXPECT(stream != NULL);
    if(stream == NULL)
    {
        return;
    }
    /* P101_TEST_CASE(p101_flockfile) */
    p101_flockfile(env, stream);
    /* P101_TEST_CASE(p101_ftrylockfile) */
    EXPECT(p101_ftrylockfile(env, stream) == 0);
    /* P101_TEST_CASE(p101_funlockfile) */
    p101_funlockfile(env, stream);
    p101_funlockfile(env, stream);

    /* P101_TEST_CASE(p101_setlinebuf) */
    p101_setlinebuf(env, stream);
    EXPECT(fclose(stream) == 0);
}

static struct p101_env *create_uncertain_env(struct p101_error *err, const char *call_name)
{
    struct p101_env *env;

    EXPECT(setenv("P101_FAULT_CALL", "1", 1) == 0);
    EXPECT(setenv("P101_FAULT_MODE", "uncertain", 1) == 0);
    EXPECT(setenv("P101_FAULT_NAME", call_name, 1) == 0);
    env = p101_env_create(err, NULL);
    EXPECT(unsetenv("P101_FAULT_CALL") == 0);
    EXPECT(unsetenv("P101_FAULT_MODE") == 0);
    EXPECT(unsetenv("P101_FAULT_NAME") == 0);
    return env;
}

P101_ATTR_SEMANTIC_ROLE("p101:fault-semantics:uncertain-native-effect")

static void test_uncertain_write_hides_completed_operation(void)
{
    static const char  payload[] = "done";
    struct p101_error *err;
    struct p101_env   *env;
    int                descriptors[2] = {-1, -1};

    err = p101_error_create(false);
    EXPECT(err != NULL);
    if(err == NULL)
    {
        return;
    }
    env = create_uncertain_env(err, "p101_write");
    EXPECT(env != NULL);
    EXPECT(pipe(descriptors) == 0);
    if(env != NULL && descriptors[0] >= 0)
    {
        char received[sizeof(payload)] = {0};

        EXPECT(p101_write(env, err, descriptors[1], payload, sizeof(payload)) == -1);
        EXPECT(p101_error_is_errno(err, ETIMEDOUT));
        EXPECT(read(descriptors[0], received, sizeof(received)) == (ssize_t)sizeof(received));
        EXPECT(memcmp(received, payload, sizeof(payload)) == 0);
    }
    if(descriptors[0] >= 0)
    {
        EXPECT(close(descriptors[0]) == 0);
        EXPECT(close(descriptors[1]) == 0);
    }
    p101_env_destroy(env);
    p101_error_destroy(err);
}

static void test_invalid_aio_inputs(const struct p101_env *env, struct p101_error *err)
{
    int     int_result;
    ssize_t size_result;

    errno      = 0;
    int_result = p101_aio_error(env, err, NULL);
    EXPECT(int_result == EINVAL);
    EXPECT(errno == EINVAL);
    EXPECT(p101_error_is_errno(err, EINVAL));
    p101_error_reset(err);

    errno      = 0;
    int_result = p101_aio_fsync(env, err, O_SYNC, NULL);
    EXPECT(int_result == -1);
    EXPECT(errno == EINVAL);
    EXPECT(p101_error_is_errno(err, EINVAL));
    p101_error_reset(err);

    errno      = 0;
    int_result = p101_aio_read(env, err, NULL);
    EXPECT(int_result == -1);
    EXPECT(errno == EINVAL);
    EXPECT(p101_error_is_errno(err, EINVAL));
    p101_error_reset(err);

    errno       = 0;
    size_result = p101_aio_return(env, err, NULL);
    EXPECT(size_result == (ssize_t)-1);
    EXPECT(errno == EINVAL);
    EXPECT(p101_error_is_errno(err, EINVAL));
    p101_error_reset(err);

    errno      = 0;
    int_result = p101_aio_suspend(env, err, NULL, 1, NULL);
    EXPECT(int_result == -1);
    EXPECT(errno == EINVAL);
    EXPECT(p101_error_is_errno(err, EINVAL));
    p101_error_reset(err);

    errno      = 0;
    int_result = p101_aio_write(env, err, NULL);
    EXPECT(int_result == -1);
    EXPECT(errno == EINVAL);
    EXPECT(p101_error_is_errno(err, EINVAL));
    p101_error_reset(err);

    errno      = 0;
    int_result = p101_lio_listio(env, err, LIO_WAIT, NULL, 1, NULL);
    EXPECT(int_result == -1);
    EXPECT(errno == EINVAL);
    EXPECT(p101_error_is_errno(err, EINVAL));
    p101_error_reset(err);
}

int main(void)
{
    struct p101_error *err;
    struct p101_env   *env;
    struct aiocb       control = {0};
    int                fd;
    int                aio_status;

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

    /* P101_TEST_CASE(p101_aio_error) */
    aio_status = p101_aio_error(env, err, &control);
    if(aio_status == EINVAL || aio_status == ENOSYS)
    {
        EXPECT(p101_error_is_errno(err, aio_status));
        p101_error_reset(err);
    }
    else
    {
        EXPECT(p101_error_has_no_error(err));
    }
    test_invalid_aio_inputs(env, err);
    test_file_locking(env);
    test_uncertain_write_hides_completed_operation();

    fd = open("/dev/null", O_WRONLY);
    EXPECT(fd >= 0);
    if(fd >= 0)
    {
        EXPECT(call_vdprintf(env, err, fd, "%s", "p101") == 4);
        EXPECT(p101_error_has_no_error(err));
        EXPECT(close(fd) == 0);
    }

    p101_env_destroy(env);
    p101_error_destroy(err);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
