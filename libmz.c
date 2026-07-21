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

#include "libmz.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef _WIN32

    #include <direct.h>

    #define mz_fseek _fseeki64
    #define mz_ftell _ftelli64

#else

    #include <sys/types.h>
    #include <sys/stat.h>

	#define mz_fseek fseeko
    #define mz_ftell ftello

#endif

struct MZ_AFI {
    uint64_t filename_len;
    char *filename;
    uint64_t file_size;
    uint64_t data_offset;
};

struct MZ_ARCHIVE {
	FILE *fp;
    uint64_t format_version;
    uint64_t file_count;
    uint64_t creation_time;
	uint64_t archive_size;
    MZ_AFI  *files;
};

struct MZ_BUILD {
	char header[2];
    uint64_t format_version;
    uint64_t file_count;
    uint64_t creation_time;
    char  **files;
};

// Using little endian format to write values as (uint64_t) into a file.
static int mz_fwrite_u64(FILE *fp, uint64_t value)
{
    uint8_t buffer[8];

    for (size_t i = 0; i < 8; i++) {
        buffer[i] = (uint8_t)((value >> (i * 8)) & 0xFFu);
    }

    if (fwrite(buffer, 1, sizeof(buffer), fp) != sizeof(buffer)) {
        return 1;
    }

    return 0;
}

// Using little endian format to read values as (uint64_t) from a file.
static int mz_fread_u64(FILE *fp, uint64_t *value)
{
	uint8_t buffer[8];

	if (fread(buffer, 1, sizeof(buffer), fp) != sizeof(buffer)) {
		return 1;
	}

	*value = 0;
	for (size_t i = 0; i < 8; i++) {
		*value |= ((uint64_t)buffer[i] << (i * 8));
	}

	return 0;
}

static int mz_sanitize_path(const char *filepath)
{
    if (strstr(filepath, "../") != NULL){
        return -1;
	}

    if (strstr(filepath, "..\\") != NULL){
        return -1;
	}

	if (strstr(filepath, "\\") != NULL){
        return -1;
	}

    if (strlen(filepath) >= 2 && filepath[1] == ':'){
        return -1;
	}

    if (filepath[0] == '/'){
        return -1;
	}

    if (strncmp(filepath, "\\\\", 2) == 0){
        return -1;
	}

    return 0;
}

static void mz_normalize_path(char *path)
{
    for(int i = 0; path[i] != '\0'; i++)
	{
		if(path[i] == '\\')
		{
			path[i] = '/';
		}
	}
}

static uint64_t mz_file_size(FILE *fp)
{
	int64_t file_pos = mz_ftell(fp);
    mz_fseek(fp, 0, SEEK_END);
    uint64_t size = (uint64_t)mz_ftell(fp);
    mz_fseek(fp, file_pos, SEEK_SET);
    return size;
}

MZ_ARCHIVE *mz_archive_open(const char *file)
{
    FILE *fp = fopen(file, "rb");
    if (!fp)
        return NULL;

    MZ_ARCHIVE *archive = calloc(1, sizeof(*archive));
    if (!archive) {
        fclose(fp);
        return NULL;
    }

    archive->fp = fp;

    char header[3];

    if (fread(header, 1, 2, fp) != 2)
        goto error;

    header[2] = '\0';

    if (strcmp(header, "MZ") != 0) goto error;

    if (mz_fread_u64(fp, &archive->format_version) != 0)
		goto error;

	if (archive->format_version != LIBMZ_FORMAT_VERSION)
		goto error;

    if (mz_fread_u64(fp, &archive->file_count) != 0)
		goto error;

	if (archive->file_count > 0) {
		archive->files = calloc(archive->file_count, sizeof(MZ_AFI));
		if (!archive->files)
			goto error;
	} else {
		archive->files = NULL;
	}

    for (uint64_t i = 0; i < archive->file_count; i++) {

        MZ_AFI *fi = &archive->files[i];

        if (mz_fread_u64(fp, &fi->filename_len) != 0)
            goto error;

		if (fi->filename_len == UINT64_MAX)
			goto error;
		
        fi->filename = malloc(fi->filename_len + 1);
        if (!fi->filename)
            goto error;
		
        if (fread(fi->filename, 1, fi->filename_len, fp) != fi->filename_len)
            goto error;

        fi->filename[fi->filename_len] = '\0';

        if (mz_fread_u64(fp, &fi->file_size) != 0)
            goto error;

        int64_t pos = mz_ftell(fp);
        if (pos < 0)
            goto error;
		
		if ((uint64_t)fi->data_offset == UINT64_MAX)
			goto error;
		
        fi->data_offset = (uint64_t)pos;

        if (mz_fseek(fp, (int64_t)fi->file_size, SEEK_CUR) != 0)
            goto error;
		
		if (fi->file_size == UINT64_MAX)
			goto error;
    }
	
	if (mz_fread_u64(fp, &archive->creation_time) != 0)
        goto error;
	
	archive->archive_size = mz_file_size(fp);

    return archive;

error:

    if (archive) {
        if (archive->files) {
            for (uint64_t i = 0; i < archive->file_count; i++)
                free(archive->files[i].filename);

            free(archive->files);
        }

        if (archive->fp)
            fclose(archive->fp);

        free(archive);
    }

    return NULL;
}

void mz_archive_close(MZ_ARCHIVE *mz)
{
    if (!mz)
        return;

    if (mz->fp)
        fclose(mz->fp);

    if (mz->files) {
        for (uint64_t i = 0; i < mz->file_count; i++) {
            free(mz->files[i].filename);
        }

        free(mz->files);
    }

    free(mz);
}

uint64_t mz_archive_file_count(const MZ_ARCHIVE *mz)
{
    return mz ? mz->file_count : UINT64_MAX;
}

uint64_t mz_archive_format_version(const MZ_ARCHIVE *mz)
{
    return mz ? mz->format_version : UINT64_MAX;
}

uint64_t mz_archive_creation_time(const MZ_ARCHIVE *mz)
{
    return mz ? mz->creation_time : UINT64_MAX;
}

uint64_t mz_archive_archive_size(const MZ_ARCHIVE *mz)
{
    return mz ? mz->archive_size : UINT64_MAX;
}

uint64_t mz_archive_filename_len_of(const MZ_AFI *file)
{
    if (!file)
        return UINT64_MAX;

    return file->filename_len;
}

const char *mz_archive_filename_of(const MZ_AFI *file)
{
    if (!file)
        return NULL;

    return file->filename;
}

uint64_t mz_archive_file_size_of(const MZ_AFI *file)
{
    if (!file)
        return UINT64_MAX;

    return file->file_size;
}

uint64_t mz_archive_data_offset_of(const MZ_AFI *file)
{
    if (!file)
        return UINT64_MAX;

    return file->data_offset;
}

const MZ_AFI *mz_archive_file_by_name(const MZ_ARCHIVE *mz, const char *filename)
{
    if (!mz || !filename)
        return NULL;

    for (uint64_t i = 0; i < mz->file_count; ++i)
        if (strcmp(mz->files[i].filename, filename) == 0)
            return &mz->files[i];

    return NULL;
}

const MZ_AFI *mz_archive_file_by_index(const MZ_ARCHIVE *mz, uint64_t index)
{
    if (!mz || index >= mz->file_count)
        return NULL;

    return &mz->files[index];
}

int mz_archive_read_file(const MZ_ARCHIVE *mz, const MZ_AFI *file, void *buffer, uint64_t size, uint64_t offset)
{
	if(!mz || !file)
		return -1;
	
	FILE *stream = mz->fp;
	if(!stream) return -1;
	
	if (!buffer)
		return -1;

	if (offset > file->file_size)
		return -1;

	if (size > file->file_size - offset)
		return -1;
	
	int64_t cur_offset = mz_ftell(stream);

	if (cur_offset < 0)
		return -1;
	
	if(mz_fseek(stream, file->data_offset + offset, SEEK_SET) != 0){
		mz_fseek(stream, cur_offset, SEEK_SET);
		return -1;
	}
	
	if(fread(buffer, 1, size, stream) != size){
		mz_fseek(stream, cur_offset, SEEK_SET);
		return -1;
	}
	mz_fseek(stream, cur_offset, SEEK_SET);
	return 0;
}

MZ_BUILD *mz_build_open(uint64_t format_version)
{
	MZ_BUILD *mz = calloc(1, sizeof(*mz));
	if(!mz) return NULL;
	
	mz->header[0] = 'M';
	mz->header[1] = 'Z';
	
	mz->format_version = format_version;
	mz->file_count = 0;
	mz->creation_time = (uint64_t)time(NULL);
	mz->files = NULL;
	return mz;
}

int mz_build_add_file(MZ_BUILD *mz, const char *filename)
{
	if(!mz || !filename) return -1;
	
	char **new_files = realloc(mz->files, sizeof(char *) * (mz->file_count + 1));
	if(!new_files) return -1;
	
	mz->files = new_files;

	uint64_t filename_len = strlen(filename);
	if (filename_len > 4096) return -1;
	mz->files[mz->file_count] = malloc(filename_len + 1);
	if(!mz->files[mz->file_count]){
		return -1;
	}
	
	strcpy(mz->files[mz->file_count], filename);
	
	mz_normalize_path(mz->files[mz->file_count]);

	if (mz_sanitize_path(mz->files[mz->file_count]) != 0) {
		free(mz->files[mz->file_count]);
		return -1;
	}
	
	mz->creation_time = (uint64_t)time(NULL);
	mz->file_count++;
	
	return 0;
}

int mz_build_write(MZ_BUILD *mz, const char *archive)
{
	if (!mz || !archive ) return -1;
	
	FILE *in = NULL;
	void *buffer = NULL;
	
	FILE *out = fopen(archive, "wb");
	if(!out) return -1;
	
	if(fwrite(mz->header, sizeof(char), 2, out) != 2){
		goto error;
	}
	if(mz_fwrite_u64(out, mz->format_version) != 0){
		goto error;
	}
	if(mz_fwrite_u64(out, mz->file_count) != 0){
		goto error;
	}
	buffer = malloc(LIBMZ_BUFFER);
	if(!buffer){
		goto error;
	}

	for(uint64_t file = 0; file < mz->file_count; file++){

		char *filename = mz->files[file];
		if(!filename){
			goto error;
		}
		
		in = fopen(filename, "rb");
		if(!in){
			goto error;
		}
		uint64_t filenamelength = strlen(filename);

		if(mz_fwrite_u64(out, filenamelength) != 0){
			goto error;
		}

		if(fwrite(filename, sizeof(char), filenamelength, out) != filenamelength){
			goto error;
		}

		uint64_t filecontentsize = mz_file_size(in);

		if(mz_fwrite_u64(out, filecontentsize) != 0){
			goto error;
		}

		uint64_t remaining = filecontentsize;
		
		while(remaining != 0){
			size_t chunk = (remaining > LIBMZ_BUFFER) ? LIBMZ_BUFFER : (size_t)remaining;
			if(fread(buffer, 1, chunk, in) != chunk){
				goto error;
			}
			if(fwrite(buffer, 1, chunk, out) != chunk){
				goto error;
			}
			remaining -= chunk;
		}
		
		fclose(in);
		in = NULL;
	}
	
	mz->creation_time = (uint64_t)time(NULL);
	if(mz_fwrite_u64(out, mz->creation_time) != 0){
		goto error;
	}
	
	fclose(out);
	free(buffer);
	return 0;

error:
    if(out){
        fclose(out);
        remove(archive);
    }
	if(in != NULL) fclose(in);
	if(buffer != NULL) free(buffer);
	return -1;
}

void mz_build_close(MZ_BUILD *mz)
{
    if (!mz)
        return;

    for (uint64_t i = 0; i < mz->file_count; i++)
        free(mz->files[i]);

    free(mz->files);
    free(mz);
}
