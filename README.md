# libmz

A small C library for reading and working with the **MZ archiving format**.

`libmz` provides an easy way to access `.mz` archive files from your own C projects without needing to use the MZ command line application.

The library allows programs to:

- Open and validate MZ archive files
- Read archive metadata
- Get information about files stored inside the archive
- Search files by name
- Access file sizes and data offsets
- Build custom tools around the MZ file format
- Build virtual archives
- Write virtual archives into disk

# Version
v0.1.0

# MZ CLI

The original MZ archiver is a command-line application for creating and extracting `.mz` files.

You can check the MZ CLI here:

https://github.com/findstring/mz


> [!WARNING]
> This library is still under development, so use it at your own risk.
>
> The API may change in future versions.
>
> Archive format versions are **not** backward or forward compatible with each other. `mz_archive_open()` will refuse to open an archive written with a different format version than the one this build of the library supports.


# About

`libmz` was created after the release of the original MZ archiver.

The original MZ software was developed as a standalone command-line application using this library. Later, I wanted to make the MZ file format usable by other programs, so this library was distributed.

The purpose of this library is to allow developers to use the MZ archive format in their own applications without depending on the MZ CLI.

# Requirements

- ISO C99 or later
- Standard C Library
- Windows and Linux supported

# Features

Currently supported:

- Open `.mz` archive files
- Validate MZ archive headers and reject archives with an unsupported format version or a malformed file table
- Read archive format version
- Read archive file count
- Read archive creation time
- Read file content
- Access stored filenames
- Get file sizes
- Get file data offsets
- Search files by filename
- Access files by index
- Build new archives in memory and write them to disk
- C and C++ compatibility
- Stores all multi-byte integers in a fixed little-endian layout, so the same archive reads correctly on little- and big-endian machines


# Example Usage

## Reading an archive

```c
#include "libmz.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    MZ_ARCHIVE *archive = mz_archive_open("example.mz");
    if (!archive) {
        fprintf(stderr, "Failed to open archive.\n");
        return EXIT_FAILURE;
    }

    printf("Archive Information\n");
    printf("-------------------\n");
    printf("Files: %" PRIu64 "\n", mz_archive_file_count(archive));
    printf("Format Version: %" PRIu64 "\n", mz_archive_format_version(archive));
    printf("Created: %" PRIu64 "\n\n", mz_archive_creation_time(archive));

    const MZ_AFI *file = mz_archive_file_by_index(archive, 0);
    if (!file) {
        fprintf(stderr, "Archive contains no files.\n");
        mz_archive_close(archive);
        return EXIT_FAILURE;
    }

    printf("First File\n");
    printf("----------\n");
    printf("Name: %s\n", mz_archive_filename_of(file));
    printf("Size: %" PRIu64 " bytes\n", mz_archive_file_size_of(file));

    uint64_t size = mz_archive_file_size_of(file);
    void *buffer = malloc((size_t)size);

    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory.\n");
        mz_archive_close(archive);
        return EXIT_FAILURE;
    }

    if (mz_archive_read_file(archive, file, buffer, size, 0) != 0) {
        fprintf(stderr, "Failed to read file contents.\n");
        free(buffer);
        mz_archive_close(archive);
        return EXIT_FAILURE;
    }

    printf("Successfully read %" PRIu64 " bytes.\n", size);

    free(buffer);
    mz_archive_close(archive);

    return EXIT_SUCCESS;
}
```

## Building an archive

```c
#include "libmz.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    MZ_BUILD *build = mz_build_open(0 /* format version */);
    if (!build) {
        fprintf(stderr, "Failed to create archive builder.\n");
        return EXIT_FAILURE;
    }

    if (mz_build_add_file(build, "notes.txt") != 0 ||
        mz_build_add_file(build, "photo.png") != 0) {
        fprintf(stderr, "Failed to add file to archive.\n");
        mz_build_close(build);
        return EXIT_FAILURE;
    }

    if (mz_build_write(build, "example.mz") != 0) {
        fprintf(stderr, "Failed to write archive.\n");
        mz_build_close(build);
        return EXIT_FAILURE;
    }

    mz_build_close(build);

    printf("Wrote example.mz\n");
    return EXIT_SUCCESS;
}
```


# Design Philosophy

The goal of `libmz` is to keep the MZ format simple and easy to use.

MZ archives are designed to be:

- Lightweight
- Fast
- Easy to parse
- Portable
- Simple to implement

The format does not use compression.

This makes MZ useful for situations where fast archive access is more important than reducing file size.


# Current Limitations

Currently not supported:

- Extracting files directly to disk (you can still read file contents into memory with `mz_archive_read_file`)
- Compression
- Encryption
- Modifying existing archives in place
- Backward or forward compatibility between archive format versions — `mz_archive_open()` requires an exact format version match
- Deep structural/checksum validation — the library checks the header, format version, and rejects a couple of known malformed-length cases, but there's no CRC or full integrity check yet

These features may be added in future versions.


# Supported Format Versions

Currently supported:

```
MZ Format Version: 0
```

Archives written with any other format version will be rejected by `mz_archive_open()`. There is currently no compatibility layer between format versions — this is a deliberate, if early-stage, design choice rather than a bug.


# Installation

Currently, the library can be added manually.

Add:

```
libmz.h
libmz.c
```

to your project and compile normally.

Example:

```
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -D_POSIX_C_SOURCE=200809L main.c libmz.c
```

> [!NOTE]
> On Linux/macOS, `-D_POSIX_C_SOURCE=200809L` is required so `fseeko`/`ftello` are visible under strict `-std=c99`. It has no effect on the Windows build, which uses `_fseeki64`/`_ftelli64` instead.


# License

MIT License

Copyright (c) 2026 Moinak Debnath


# Status

🚧 Under Development

The API is still evolving and may change before the first stable release.

Suggestions and contributions are welcome.