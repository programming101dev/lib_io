#ifndef LIBP101_IO_IO_H
#define LIBP101_IO_IO_H

/*
 * Copyright 2026 D'Arcy Smith.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 */

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

#ifdef __cplusplus
extern "C"
{
#endif

    int     p101_aio_cancel(const struct p101_env *env, struct p101_error *err, int fildes, struct aiocb *aiocbp);
    int     p101_aio_error(const struct p101_env *env, const struct aiocb *aiocbp);
    int     p101_aio_fsync(const struct p101_env *env, struct p101_error *err, int op, struct aiocb *aiocbp);
    int     p101_aio_read(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp);
    ssize_t p101_aio_return(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp);
    int     p101_aio_suspend(const struct p101_env *env, struct p101_error *err, const struct aiocb *const list[], int nent, const struct timespec *timeout);
    int     p101_aio_write(const struct p101_env *env, struct p101_error *err, struct aiocb *aiocbp);
    int     p101_close(const struct p101_env *env, struct p101_error *err, int fildes);
    int     p101_creat(const struct p101_env *env, struct p101_error *err, const char *path, mode_t mode);
    int     p101_dup(const struct p101_env *env, struct p101_error *err, int fildes);
    int     p101_dup2(const struct p101_env *env, struct p101_error *err, int fildes, int fildes2);
    int     p101_fcntl(const struct p101_env *env, struct p101_error *err, int fildes, int cmd, ...);
    FILE   *p101_fdopen(const struct p101_env *env, struct p101_error *err, int fildes, const char *mode) P101_ATTR_WARN_UNUSED_RESULT;
    int     p101_fileno(const struct p101_env *env, struct p101_error *err, FILE *stream);
    void    p101_flockfile(const struct p101_env *env, FILE *file);
    FILE   *p101_fmemopen(const struct p101_env *env, struct p101_error *err, void *restrict buf, size_t size, const char *restrict mode) P101_ATTR_WARN_UNUSED_RESULT;
    int     p101_fpurge(const struct p101_env *env, struct p101_error *err, FILE *stream);
    int     p101_fseeko(const struct p101_env *env, struct p101_error *err, FILE *stream, off_t offset, int whence);
    off_t   p101_ftello(const struct p101_env *env, struct p101_error *err, FILE *stream);
    int     p101_ftrylockfile(const struct p101_env *env, FILE *file);
    void    p101_funlockfile(const struct p101_env *env, FILE *file);
    int     p101_getc_unlocked(const struct p101_env *env, struct p101_error *err, FILE *stream);
    int     p101_getchar_unlocked(const struct p101_env *env, struct p101_error *err);
    ssize_t p101_getdelim(const struct p101_env *env, struct p101_error *err, char **restrict lineptr, size_t *restrict n, int delimiter, FILE *restrict stream);
    ssize_t p101_getline(const struct p101_env *env, struct p101_error *err, char **restrict lineptr, size_t *restrict n, FILE *restrict stream);
    int     p101_lio_listio(const struct p101_env *env, struct p101_error *err, int mode, struct aiocb *restrict const list[restrict], int nent, struct sigevent *restrict sig);
    int     p101_lockf(const struct p101_env *env, struct p101_error *err, int fildes, int function, off_t size);
    off_t   p101_lseek(const struct p101_env *env, struct p101_error *err, int fildes, off_t offset, int whence);
    int     p101_open(const struct p101_env *env, struct p101_error *err, const char *path, int oflag, ...);
    FILE   *p101_open_memstream(const struct p101_env *env, struct p101_error *err, char **bufp, size_t *sizep) P101_ATTR_WARN_UNUSED_RESULT;
    int     p101_openat(const struct p101_env *env, struct p101_error *err, int fd, const char *path, int oflag, ...);
    int     p101_poll(const struct p101_env *env, struct p101_error *err, struct pollfd fds[], nfds_t nfds, int timeout);
    ssize_t p101_pread(const struct p101_env *env, struct p101_error *err, int fildes, void *buf, size_t nbyte, off_t offset);
    int     p101_pselect(const struct p101_env *env, struct p101_error *err, int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, const struct timespec *restrict timeout, const sigset_t *restrict sigmask);
    int     p101_putc_unlocked(const struct p101_env *env, struct p101_error *err, int c, FILE *stream);
    int     p101_putchar_unlocked(const struct p101_env *env, struct p101_error *err, int c);
    ssize_t p101_pwrite(const struct p101_env *env, struct p101_error *err, int fildes, const void *buf, size_t nbyte, off_t offset);
    ssize_t p101_read(const struct p101_env *env, struct p101_error *err, int fildes, void *buf, size_t nbyte);
    ssize_t p101_readv(const struct p101_env *env, struct p101_error *err, int fildes, const struct iovec *iov, int iovcnt);
    int     p101_select(const struct p101_env *env, struct p101_error *err, int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, struct timeval *restrict timeout);
    int     p101_setbuffer(const struct p101_env *env, struct p101_error *err, FILE *stream, char *buf, size_t size);
    void    p101_setlinebuf(const struct p101_env *env, FILE *stream);
    int     p101_vdprintf(const struct p101_env *env, struct p101_error *err, int fildes, const char *restrict format, va_list ap) P101_ATTR_PRINTF(4, 0);
    ssize_t p101_write(const struct p101_env *env, struct p101_error *err, int fildes, const void *buf, size_t nbyte);
    ssize_t p101_writev(const struct p101_env *env, struct p101_error *err, int fildes, const struct iovec *iov, int iovcnt);

#ifdef __cplusplus
}
#endif

#endif    // LIBP101_IO_IO_H
