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
- Build virtual archives for writing data from a file
- Build virtual archives for writing data from memory
- Write virtual archives into disk

# Version
v1.0.0

# Changelog [0.1.0 -> 1.0.0]
- Fully Changed API with more better and convinient function names.
- New Memory-Save Mode is Introduced to save data directly from memory to a File.
- More Bound Checks and Limits.

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

### For File Save Mode
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
- Build new archives in memory and write them to disk **This mode for writing data from reading a file**
- C and C++ compatibility
- Stores all multi-byte integers in a fixed little-endian layout, so the same archive reads correctly on little- and big-endian machines

### For Memory Save Mode
- Open `.*` archive files
- Validate MZ archive headers and reject archives with an unsupported format version or a malformed file table
- Read archive format version
- Read archive data count
- Read archive creation time
- Read data content
- Access stored datanames
- Get data sizes
- Get data offsets
- Search content by dataname
- Access content by index
- Build new archives in memory and write them to disk **This mode for writing data directly from memory**
- C and C++ compatibility
- Stores all multi-byte integers in a fixed little-endian layout, so the same archive reads correctly on little- and big-endian machines

# Example Usage

## Writing and Reading an archive in File Save Mode

### Writing Files

```c
#include "libmz.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    MZ_FILESAVE *save = mz_filesave_open(0 /* format version */);

    if (save == NULL) {
        fprintf(stderr, "Failed to create archive builder.\n");
        return EXIT_FAILURE;
    }

    if (mz_filesave_add_file(save, "notes.txt") != 0 ||
        mz_filesave_add_file(save, "photo.png") != 0) {

        fprintf(stderr, "Failed to add file to archive.\n");
        mz_filesave_close(save);
        return EXIT_FAILURE;
    }

    if (mz_filesave_write(save, "example.mz") != 0) {
        fprintf(stderr, "Failed to write archive.\n");
        mz_filesave_close(save);
        return EXIT_FAILURE;
    }

    mz_filesave_close(save);

    printf("Wrote example.mz\n");

    return EXIT_SUCCESS;
}
```

### Reading File
```c
#include "libmz.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    MZ_ARCHIVE *archive = mz_archive_open("example.mz");

    if (archive == NULL) {
        fprintf(stderr, "Failed to open archive.\n");
        return EXIT_FAILURE;
    }

    printf("Archive Information\n");
    printf("-------------------\n");
    printf("Data: %" PRIu64 "\n", mz_archive_data_count(archive));
    printf("Format Version: %" PRIu64 "\n",
           mz_archive_format_version(archive));
    printf("Created: %" PRIu64 "\n\n",
           mz_archive_creation_time(archive));

    const MZ_CONTENT *content =
        mz_archive_content_by_index(archive, 0);

    if (content == NULL) {
        fprintf(stderr, "Archive contains no content.\n");
        mz_archive_close(archive);
        return EXIT_FAILURE;
    }

    printf("First Content\n");
    printf("-------------\n");
    printf("Name: %s\n", mz_archive_dataname_of(content));

    uint64_t size = mz_archive_content_size_of(content);

    printf("Size: %" PRIu64 " bytes\n", size);

    void *buffer = malloc((size_t)size);

    if (buffer == NULL && size != 0) {
        fprintf(stderr, "Failed to allocate memory.\n");
        mz_archive_close(archive);
        return EXIT_FAILURE;
    }

    if (mz_archive_read_content(
            archive,
            content,
            buffer,
            size,
            0) != 0) {

        fprintf(stderr, "Failed to read content.\n");
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

## Writing and Reading an archive in Memory Save Mode

### Writing Game Data
```c
#include "libmz.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    MZ_MEMSAVE *save = mz_memsave_open(0 /* format version */);

    if (save == NULL) {
        fprintf(stderr, "Failed to create memory archive.\n");
        return EXIT_FAILURE;
    }

    /* Player Health */
    uint64_t player_health = 100;

    if (mz_memsave_add_data(
            save,
            "Player Health",
            sizeof(player_health),
            &player_health) != 0) {

        fprintf(stderr, "Failed to add Player Health.\n");
        mz_memsave_close(save);
        return EXIT_FAILURE;
    }

    /* Player Score */
    uint64_t player_score = 5000;

    if (mz_memsave_add_data(
            save,
            "Player Score",
            sizeof(player_score),
            &player_score) != 0) {

        fprintf(stderr, "Failed to add Player Score.\n");
        mz_memsave_close(save);
        return EXIT_FAILURE;
    }

    /* Player Name */
    const char *player_name = "Player1";

    if (mz_memsave_add_data(
            save,
            "Player Name",
            strlen(player_name) + 1,
            player_name) != 0) {

        fprintf(stderr, "Failed to add Player Name.\n");
        mz_memsave_close(save);
        return EXIT_FAILURE;
    }

    /* Game Version */
    const char *game_version = "1.0.0";

    if (mz_memsave_add_data(
            save,
            "Game Version",
            strlen(game_version) + 1,
            game_version) != 0) {

        fprintf(stderr, "Failed to add Game Version.\n");
        mz_memsave_close(save);
        return EXIT_FAILURE;
    }

    /* Write archive */
    if (mz_memsave_write(save, "game.save") != 0) {
        fprintf(stderr, "Failed to write memory archive.\n");
        mz_memsave_close(save);
        return EXIT_FAILURE;
    }

    mz_memsave_close(save);

    printf("Wrote game.save\n");

    return EXIT_SUCCESS;
}
```
### Reading Game Data
```c
#include "libmz.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	// Load the Saved Data File in Memory
    MZ_ARCHIVE *game_data = mz_archive_open("game.save");
	if (!game_data){
		printf("Unable to load game data file\n");
		return -1;
	}
	
	// Data names of the data required
	char *data_name1 = "Player Health";
	char *data_name2 = "Player Score";
	
	// Getting content struct from name 
	const MZ_CONTENT *player_health_data = mz_archive_content_by_name(game_data, data_name1);
	if(!player_health_data){
		printf("Player Health Data Not Found\n");
		return -1;
	}
	
	// Reading Player Health Data in buffer1
	uint8_t buffer1[8] = {0};
	if(mz_archive_read_content(game_data, player_health_data, buffer1, 8, 0) == -1){
		printf("Unable to Read Player Health Data \n");
		return -1;
	}
	
	// Converting byte sequence to a uint64_t as Player Health Value
	uint64_t health_value = 0;
	memcpy(&health_value, buffer1, sizeof(health_value));
	
	// Printing Player Health Data
	printf("%s : %lld\n", data_name1, health_value);
	
	// Getting content struct from name 
	const MZ_CONTENT *player_score_data = mz_archive_content_by_name(game_data, data_name2);
	if(!player_score_data){
		printf("Player Score Data Not Found\n");
		return -1;
	}
	
	// Reading Player Score Data in buffer2
	uint8_t buffer2[8] = {0};
	if(mz_archive_read_content(game_data, player_score_data, buffer2, 8, 0) == -1){
		printf("Unable to Read Player Score Data \n");
		return -1;
	}
	
	// Converting byte sequence to a uint64_t as Player Score Value
	uint64_t score_value = 0;
	memcpy(&score_value, buffer2, sizeof(score_value));
	
	// Printing Player Score Data
	printf("%s : %lld\n", data_name2, score_value);

    return 0;
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
- Usable for Indie Projects

The format does not use compression.

This makes MZ useful for situations where fast archive access is more important than reducing file size.


# Current Limitations

Currently not supported:

- Extracting data directly to disk (you can still read file contents into memory with `mz_archive_read_content`)
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

🚧 Development is Good Going

Suggestions and contributions are welcome.
