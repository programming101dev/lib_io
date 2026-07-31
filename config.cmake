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
        src/posix/aio.c
        src/posix/fcntl.c
        src/posix/poll.c
        src/posix/stdio.c
        src/posix/sys/select.c
        src/posix/unistd.c
        src/posix_xsi/sys/uio.c
        src/posix_xsi/unistd.c
        src/unix/stdio.c
)
set(p101_io_HEADERS
        include/p101_io/io.h
)
set(p101_io_LINK_LIBRARIES
        p101_error
        p101_env
        p101_tool_event
        p101_c
)

