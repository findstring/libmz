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

    MZ_ARCHIVE , MZ_FILESAVE and MZ_MEMSAVE objects are NOT thread-safe.
    Do not access the same object concurrently from multiple threads.
    If concurrent access is required, open the archive separately
    in each thread.
*/

#ifndef LIBMZ_H
#define LIBMZ_H

#include <stdint.h>

#define LIBMZ_API // For future shared - libraries

// For Memory
#define LIBMZ_BUFFER (1024 * 1024)
#define LIBMZ_MAX_DATANAME_LENGTH 4096 // 4 KB
#define LIBMZ_MAX_DATA_LENGTH 6144 // Only for MZ_MEMSAVE mode as 6 KB
#define LIBMZ_MAX_DATA_COUNT 2000  // Only for MZ_MEMSAVE mode

#define LIBMZ_VERSION "2.0.0"
#define LIBMZ_VERSION_MAJOR 2
#define LIBMZ_VERSION_MINOR 0
#define LIBMZ_VERSION_PATCH 0

#define LIBMZ_FORMAT_VERSION 0

#ifdef __cplusplus
extern "C" {
#endif

// Opaque struct for holding Archiving data info in memory
typedef struct MZ_ARCHIVE MZ_ARCHIVE;

// Opaque struct for holding content data present inside an Archive
typedef struct MZ_CONTENT MZ_CONTENT;

// Opaque struct for building an archive in memory
typedef struct MZ_FILESAVE MZ_FILESAVE;

// Opaque struct for building an archive for data [ not file ] in memory
typedef struct MZ_MEMSAVE MZ_MEMSAVE;

/**
	Function to load a valid MZ file
	
	@Param : 
		filename : name of the archive file to be opened
	@Returns :
		On success :- pointer to an allocated MZ_ARCHIVE struct
		On failure :- NULL
**/
LIBMZ_API MZ_ARCHIVE *mz_archive_open(const char *file);

/**
	Function to close a previously opened archive
	
	@Param :
		mz : MZ_ARCHIVE struct which you want to unload
**/
LIBMZ_API void mz_archive_close(MZ_ARCHIVE *mz);

/**
	Function to get number of content archived in a Valid MZ file
	
	@Param :
		mz : MZ_ARCHIVE struct
	
	@Returns :
		On success :- number of archived content entries
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_archive_content_count(const MZ_ARCHIVE *mz);

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
	Function to get the data name length of a content
 	
	@Param :
		content : MZ_CONTENT struct
		
	@Returns :
		On success :- data name length
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_content_dataname_len_of(const MZ_CONTENT *content);

/**
	Function to get the data name of a content
 
	@Param :
		content : MZ_CONTENT struct
		
	@Returns :
		On success :- data name [ Do not free the dataname , as it is a part of mz struct ]
		On failure :- NULL
**/
LIBMZ_API const char *mz_content_dataname_of(const MZ_CONTENT *content);

/**
	Function to get the data size of a content
 
	@Param :
		content : MZ_CONTENT struct
		
	@Returns :
		On success :- datasize
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_content_data_size_of(const MZ_CONTENT *content);

/**
	Function to get the data offset of a content
 
	@Param :
		content : MZ_CONTENT struct
		
	@Returns :
		On success :- offset
		On failure :- UINT64_MAX
**/
LIBMZ_API uint64_t mz_content_data_offset_of(const MZ_CONTENT *content);

/**
	Function to get content struct from a loaded archive using its name
 
	@Param :
		mz       : MZ_ARCHIVE struct
		dataname : data name to search for
		
	@Returns :
		On success :- pointer to content information [ The pointer remains valid until mz_archive_close() is called.]
		On failure :- NULL
**/
LIBMZ_API const MZ_CONTENT *mz_archive_content_by_name(const MZ_ARCHIVE *mz, const char *dataname);

/**
	Function to get content struct from a loaded archive using its index
 
	@Param :
		mz    : MZ_ARCHIVE struct
		index : zero-based content index
		
	@Returns :
		On success :- pointer to content information [ The pointer remains valid until mz_archive_close() is called.]
		On failure :- NULL
**/
LIBMZ_API const MZ_CONTENT *mz_archive_content_by_index(const MZ_ARCHIVE *mz, uint64_t index);

/**
	Function to read data of a content using data offset from the archive file 
	
	@Param :
		mz      : loaded archive
		content : archive-owned content descriptor
		buffer  : destination buffer
		size    : number of bytes to read
		offset  : byte offset inside the archived file (0 <= offset <= data size)

	@Returns :
		On success :- 0
		On failure :- -1
**/
LIBMZ_API int mz_archive_read_content_data(const MZ_ARCHIVE *mz, const MZ_CONTENT *content, void *buffer, uint64_t size, uint64_t offset);

/**
	Function to initialize a virtual archive for file data storage

	@Param :
		format_version : archive format version

	@Returns :
		On success :- pointer to the initialized MZ_FILESAVE struct
		On failure :- NULL
**/
LIBMZ_API MZ_FILESAVE *mz_filesave_open(uint64_t format_version);

/**
	Function to add a file to a virtual archive

	@Param :
		mz       : MZ_FILESAVE struct
		filename : path of the file to be added to the archive

	@Returns :
		On success :- 0
		On failure :- -1
**/
LIBMZ_API int mz_filesave_add_file(MZ_FILESAVE *mz, const char *filename);

/**
	Function to write the virtual archive into disk in mz format

	@Param :
		mz      : MZ_FILESAVE struct
		archive : output archive filename

	@Returns :
		On success :- 0
		On failure :- -1
**/
LIBMZ_API int mz_filesave_write(MZ_FILESAVE *mz, const char *archive);

/**
	Function to destroy a virtual archive

	@Param :
		mz : MZ_FILESAVE struct
**/
LIBMZ_API void mz_filesave_close(MZ_FILESAVE *mz);

/**
	Function to initialize a virtual archive for memory data storage

	@Param :
		format_version : archive format version

	@Returns :
		On success :- pointer to the initialized MZ_MEMSAVE struct
		On failure :- NULL
**/
LIBMZ_API MZ_MEMSAVE *mz_memsave_open(uint64_t format_version);

/**
	Function to add data to the virtual archive

	@Param :
		mz        : MZ_MEMSAVE struct
		data_name : name of the content data to be stored
		data_size : size of the content data
		data      : content data

	@Returns :
		On success :- 0
		On failure :- -1
**/
LIBMZ_API int mz_memsave_add_data(MZ_MEMSAVE *mz, const char *data_name, uint64_t data_size, const void *data);

/**
	Function to write the virtual archive into disk

	@Param :
		mz      : MZ_MEMSAVE struct
		archive : output archive filename

	@Returns :
		On success :- 0
		On failure :- -1
**/
LIBMZ_API int mz_memsave_write(MZ_MEMSAVE *mz, const char *archive);

/**
	Function to destroy the MZ_MEMSAVE virtual archive

	@Param :
		mz : MZ_MEMSAVE struct
**/
LIBMZ_API void mz_memsave_close(MZ_MEMSAVE *mz);

#ifdef __cplusplus
}
#endif

#endif