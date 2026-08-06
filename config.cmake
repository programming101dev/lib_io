# Project metadata
set(PROJECT_NAME "p101_io")
set(PROJECT_VERSION "0.0.1")
set(PROJECT_DESCRIPTION "Descriptor, stream, asynchronous, vectored, and multiplexed I/O")
set(PROJECT_LANGUAGE "C")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(STANDARD_FLAGS
        -D_POSIX_C_SOURCE=200809L
        -D_XOPEN_SOURCE=700
        -Werror
)
set(DARWIN_STANDARD_FLAGS -D_DARWIN_C_SOURCE)
set(LINUX_STANDARD_FLAGS -D_GNU_SOURCE)
set(BSD_STANDARD_FLAGS -D_BSD_SOURCE -D__BSD_VISIBLE)

set(LIBRARY_TARGETS p101_io)
set(p101_io_SOURCES
        src/aio.c
        src/fcntl.c
        src/poll.c
        src/stdio.c
        src/sys/select.c
        src/sys/uio.c
        src/unistd.c
)
set(p101_io_HEADERS
        include/p101_io/p101_aio.h
        include/p101_io/p101_fcntl.h
        include/p101_io/p101_poll.h
        include/p101_io/p101_stdio.h
        include/p101_io/p101_unistd.h
        include/p101_io/sys/p101_select.h
        include/p101_io/sys/p101_uio.h
)
set(p101_io_LINK_LIBRARIES
        p101_error
        p101_env
        p101_c
)

