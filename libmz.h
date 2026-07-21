/*
	MIT License

	Copyright (c) 2026 Moinak Debnath

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

*/

/*
    Thread Safety

    MZ_ARCHIVE and MZ_BUILD objects are NOT thread-safe.
    Do not access the same object concurrently from multiple threads.
    If concurrent access is required, open the archive separately
    in each thread.
*/

#ifndef LIBMZ_H
#define LIBMZ_H

#include <stdint.h>

#define LIBMZ_API // For future shared - libraries

#define LIBMZ_BUFFER (1024 * 1024)

#define LIBMZ_VERSION "0.1.0"
#define LIBMZ_VERSION_MAJOR 0
#define LIBMZ_VERSION_MINOR 1
#define LIBMZ_VERSION_PATCH 0

#define LIBMZ_FORMAT_VERSION 0

#ifdef __cplusplus
extern "C" {
#endif

// Opaque struct for holding Archiving data info in memory
typedef struct MZ_ARCHIVE MZ_ARCHIVE;

// Opaque struct for holding file data present inside an Archive
typedef struct MZ_AFI MZ_AFI;

// Opaque struct for building an archive in memory
typedef struct MZ_BUILD MZ_BUILD;

/**
	Function to load a valid MZ file
	
	@Param : 
		filename : name of the archive file to be opened
	@Returns :
		On success :- pointer to the loaded MZ_ARCHIVE struct
		On failure :- NULL
**/
LIBMZ_API MZ_ARCHIVE *mz_archive_open(const char *filename);

/**
	Function to close a previously opened MZ archive
	
	@Param :
		mz : MZ_ARCHIVE struct which you want to unload
**/
LIBMZ_API void mz_archive_close(MZ_ARCHIVE *mz);

/**
	Function to get number of files archived in a Valid MZ file
	
	@Param :
		mz : MZ_ARCHIVE struct
	
	@Returns :
		On success :- number of files
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_archive_file_count(const MZ_ARCHIVE *mz);

/**
	Function to get archive format version
 	
	@Param :
		mz : MZ_ARCHIVE struct
		
	@Returns :
		On success :- format version
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_archive_format_version(const MZ_ARCHIVE *mz);

/**
	Function to get archive creation time
 	
	@Param :
		mz : MZ_ARCHIVE struct
		
	@Returns :
		On success :- seconds passed after 1st January 1970 ( Epoch )
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_archive_creation_time(const MZ_ARCHIVE *mz);

/**
	Function to get archive size
 	
	@Param :
		mz : MZ_ARCHIVE struct
		
	@Returns :
		On success :- archive size
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_archive_archive_size(const MZ_ARCHIVE *mz);

/**
	Function to get the filename length of a loaded file
 	
	@Param :
		file : MZ_AFI struct
		
	@Returns :
		On success :- file name length
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_archive_filename_len_of(const MZ_AFI *file);

/**
	Function to get the filename of a loaded file
 
	@Param :
		file : MZ_AFI struct
		
	@Returns :
		On success :- file name [ Do not free the filename , as it is a part of mz struct ]
		On failure :- NULL
**/
LIBMZ_API const char *mz_archive_filename_of(const MZ_AFI *file);

/**
	Function to get the filesize of a loaded file
 
	@Param :
		file : MZ_AFI struct
		
	@Returns :
		On success :- filesize
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_archive_file_size_of(const MZ_AFI *file);

/**
	Function to get the filecontent offset of a loaded file in the archive
 
	@Param :
		file : MZ_AFI struct
		
	@Returns :
		On success :- offset
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_archive_data_offset_of(const MZ_AFI *file);

/**
	Function to get file from a loaded archive using its name
 
	@Param :
		mz       : MZ_ARCHIVE struct
		filename : file name to search for
		
	@Returns :
		On success :- pointer to file information [ The pointer remains valid until mz_close() is called.]
		On failure :- NULL
**/
LIBMZ_API const MZ_AFI *mz_archive_file_by_name(const MZ_ARCHIVE *mz, const char *filename);

/**
	Function to get file from a loaded archive using its index
 
	@Param :
		mz    : MZ_ARCHIVE struct
		index : zero-based file index
		
	@Returns :
		On success :- pointer to file information [ The pointer remains valid until mz_close() is called.]
		On failure :- NULL
**/
LIBMZ_API const MZ_AFI *mz_archive_file_by_index(const MZ_ARCHIVE *mz, uint64_t index);

/**
	Function to read the data of a file from the archive
	
	@Param :
		mz     : loaded archive
		file   : archive-owned file descriptor
		buffer : destination buffer
		size   : number of bytes to read
		offset : byte offset inside the archived file (0 <= offset <= file size)

	@Returns :
		On success :- 0
		On failure :- -1
**/
LIBMZ_API int mz_archive_read_file(const MZ_ARCHIVE *mz, const MZ_AFI *file, void *buffer, uint64_t size, uint64_t offset);

/**
	Function to initialize a virtual archive

	@Param :
		format_version : archive format version

	@Returns :
		On success :- pointer to the initialized MZ_BUILD struct
		On failure :- NULL
**/
LIBMZ_API MZ_BUILD *mz_build_open(uint64_t format_version);

/**
	Function to add a file to a virtual archive

	@Param :
		mz       : MZ_BUILD struct
		filename : path of the file to be added to the archive

	@Returns :
		On success :- 0
		On failure :- -1
**/
LIBMZ_API int mz_build_add_file(MZ_BUILD *mz, const char *filename);

/**
	Function to write the virtual archive into disk

	@Param :
		mz      : MZ_BUILD struct
		archive : output archive filename

	@Returns :
		On success :- 0
		On failure :- -1
**/
LIBMZ_API int mz_build_write(MZ_BUILD *mz, const char *archive);

/**
	Function to destroy a virtual archive

	@Param :
		mz : MZ_BUILD struct
**/
LIBMZ_API void mz_build_close(MZ_BUILD *mz);

#ifdef __cplusplus
}
#endif

#endif