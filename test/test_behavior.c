#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <p101_env/env.h>
#include <p101_error/error.h>
#include <p101_io/io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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
    test_file_locking(env);

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
