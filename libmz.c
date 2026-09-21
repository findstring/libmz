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

struct MZ_CONTENT {
	MZ_ARCHIVE *owner;
    uint64_t dataname_len;
    char *dataname;
    uint64_t data_size;
    uint64_t data_offset;
};

struct MZ_ARCHIVE {
	FILE *fp;
    uint64_t format_version;
    uint64_t data_count;
    uint64_t creation_time;
	uint64_t archive_size;
    MZ_CONTENT  *data;
};

struct MZ_FILESAVE {
	char header[2];
    uint64_t format_version;
    uint64_t file_count;
    uint64_t creation_time;
    char  **files;
};

struct MZ_MEMSAVE {
	char header[2];
	uint64_t format_version;
	uint64_t data_count;
	uint64_t creation_time;
	uint64_t *data_sizes;
	char **data_names;
	void **data;
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

    if (mz_fread_u64(fp, &archive->data_count) != 0)
		goto error;
	
	if (archive->data_count >= LIBMZ_MAX_DATA_COUNT)
		goto error;
	
	if (archive->data_count > 0) {
		archive->data = calloc(archive->data_count, sizeof(MZ_CONTENT));
		if (!archive->data)
			goto error;
	} else {
		archive->data = NULL;
	}

    for (uint64_t i = 0; i < archive->data_count; i++) {

        MZ_CONTENT *content = &archive->data[i];

        if (mz_fread_u64(fp, &content->dataname_len) != 0)
            goto error;

		if (content->dataname_len == UINT64_MAX || content->dataname_len >= LIBMZ_MAX_DATANAME_LENGTH)
			goto error;
		
        content->dataname = malloc(content->dataname_len + 1);
        if (!content->dataname)
            goto error;
		
        if (fread(content->dataname, 1, content->dataname_len, fp) != content->dataname_len)
            goto error;

        content->dataname[content->dataname_len] = '\0';

        if (mz_fread_u64(fp, &content->data_size) != 0)
            goto error;

        int64_t pos = mz_ftell(fp);
        if (pos < 0)
            goto error;
		
		content->data_offset = (uint64_t)pos;
		
		if ((uint64_t)content->data_offset == UINT64_MAX)
			goto error;

		if (content->data_size == UINT64_MAX)
			goto error;
		
        if (mz_fseek(fp, (int64_t)content->data_size, SEEK_CUR) != 0)
            goto error;
		
		content->owner = archive;
    }
	
	if (mz_fread_u64(fp, &archive->creation_time) != 0)
        goto error;
	
	archive->archive_size = mz_file_size(fp);

    return archive;

error:

    if (archive) {
        if (archive->data) {
            for (uint64_t i = 0; i < archive->data_count; i++)
                free(archive->data[i].dataname);

            free(archive->data);
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

    if (mz->data) {
        for (uint64_t i = 0; i < mz->data_count; i++) {
            free(mz->data[i].dataname);
        }

        free(mz->data);
    }

    free(mz);
}

uint64_t mz_archive_data_count(const MZ_ARCHIVE *mz)
{
    return mz ? mz->data_count : UINT64_MAX;
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

uint64_t mz_archive_dataname_len_of(const MZ_CONTENT *content)
{
    if (!content)
        return UINT64_MAX;

    return content->dataname_len;
}

const char *mz_archive_dataname_of(const MZ_CONTENT *content)
{
    if (!content)
        return NULL;

    return content->dataname;
}

uint64_t mz_archive_content_size_of(const MZ_CONTENT *content)
{
    if (!content)
        return UINT64_MAX;

    return content->data_size;
}

uint64_t mz_archive_data_offset_of(const MZ_CONTENT *content)
{
    if (!content)
        return UINT64_MAX;

    return content->data_offset;
}

const MZ_CONTENT *mz_archive_content_by_name(const MZ_ARCHIVE *mz, const char *dataname)
{
    if (!mz || !dataname)
        return NULL;

    for (uint64_t i = 0; i < mz->data_count; ++i)
        if (strcmp(mz->data[i].dataname, dataname) == 0)
            return &mz->data[i];

    return NULL;
}

const MZ_CONTENT *mz_archive_content_by_index(const MZ_ARCHIVE *mz, uint64_t index)
{
    if (!mz || index >= mz->data_count)
        return NULL;

    return &mz->data[index];
}

int mz_archive_read_content(const MZ_ARCHIVE *mz, const MZ_CONTENT *content, void *buffer, uint64_t size, uint64_t offset)
{
	if(!mz || !content)
		return -1;
	
	if (content->owner != mz)
		return -1;
	
	FILE *stream = mz->fp;
	if(!stream) return -1;
	
	if (!buffer)
		return -1;

	if (offset > content->data_size)
		return -1;

	if (size > content->data_size - offset)
		return -1;
	
	int64_t cur_offset = mz_ftell(stream);

	if (cur_offset < 0)
		return -1;
	
	if (offset > content->data_size)
		return -1;

	if (size > content->data_size - offset)
		return -1;
	
	if(mz_fseek(stream, content->data_offset + offset, SEEK_SET) != 0){
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

MZ_FILESAVE *mz_filesave_open(uint64_t format_version)
{
	if (format_version != LIBMZ_FORMAT_VERSION) return NULL;

	MZ_FILESAVE *mz = calloc(1, sizeof(*mz));
	if(!mz) return NULL;
	
	mz->header[0] = 'M';
	mz->header[1] = 'Z';
	
	mz->format_version = format_version;
	mz->file_count = 0;
	mz->creation_time = (uint64_t)time(NULL);
	mz->files = NULL;
	return mz;
}

int mz_filesave_add_file(MZ_FILESAVE *mz, const char *filename)
{
	if(!mz || !filename) return -1;
	
	uint64_t filename_len = strlen(filename);
	if (filename_len >= LIBMZ_MAX_DATANAME_LENGTH) return -1;
	
	char **new_files = realloc(mz->files, sizeof(char *) * (mz->file_count + 1));
	if(!new_files) return -1;
	
	mz->files = new_files;

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

int mz_filesave_write(MZ_FILESAVE *mz, const char *archive)
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

void mz_filesave_close(MZ_FILESAVE *mz)
{
    if (!mz)
        return;

    for (uint64_t i = 0; i < mz->file_count; i++)
        free(mz->files[i]);

    free(mz->files);
    free(mz);
}

MZ_MEMSAVE *mz_memsave_open(uint64_t format_version)
{
	if (format_version != LIBMZ_FORMAT_VERSION) return NULL;
	
	MZ_MEMSAVE *mz = calloc(1, sizeof(*mz));
	if(!mz) return NULL;
	
	mz->header[0] = 'M';
	mz->header[1] = 'Z';
	
	mz->format_version = format_version;
	mz->data_count = 0;
	mz->creation_time = (uint64_t)time(NULL);
	mz->data_sizes = NULL;
	mz->data_names = NULL;
	mz->data = NULL;
	return mz;
}

int mz_memsave_add_data(MZ_MEMSAVE *mz, const char *data_name, uint64_t data_size, const void *data)
{
	if(!mz || !data) return -1;
	if(data_size == 0 || data_size >= LIBMZ_MAX_DATA_LENGTH) return -1;
	if(!data_name) return -1;
	if(mz->data_count >= LIBMZ_MAX_DATA_COUNT) return -1;
	if(strlen(data_name) == 0 || strlen(data_name) >= LIBMZ_MAX_DATANAME_LENGTH) return -1;
	
	uint64_t *new_data_sizes = realloc(mz->data_sizes, sizeof(uint64_t) * (mz->data_count + 1));
	if(!new_data_sizes) return -1;
	
	mz->data_sizes = new_data_sizes;
	
	char **new_data_names = realloc(mz->data_names, sizeof(char *) * (mz->data_count + 1));
	if(!new_data_names) return -1;
	
	mz->data_names = new_data_names;
	
	void **new_data = realloc(mz->data, sizeof(void *) * (mz->data_count + 1));
	if(!new_data) return -1;
	
	mz->data = new_data;

	mz->data[mz->data_count] = malloc(data_size);
	if(!mz->data[mz->data_count]){
		return -1;
	}
	
	mz->data_names[mz->data_count] = malloc(strlen(data_name) + 1);
	if(!mz->data_names[mz->data_count]){
		return -1;
	}
	
	memcpy(mz->data[mz->data_count], data, data_size);
	mz->data_sizes[mz->data_count] = data_size;
	strcpy(mz->data_names[mz->data_count], data_name);
	
	mz->creation_time = (uint64_t)time(NULL);
	mz->data_count++;
	
	return 0;
}

int mz_memsave_write(MZ_MEMSAVE *mz, const char *archive)
{
	if (!mz || !archive ) return -1;
		
	FILE *out = fopen(archive, "wb");
	if(!out) return -1;
	
	if(fwrite(mz->header, sizeof(char), 2, out) != 2){
		goto error;
	}
	if(mz_fwrite_u64(out, mz->format_version) != 0){
		goto error;
	}
	if(mz_fwrite_u64(out, mz->data_count) != 0){
		goto error;
	}

	for(uint64_t i = 0; i < mz->data_count; i++){

		char *data_name = mz->data_names[i];
		if(!data_name){
			goto error;
		}

		uint64_t datanamelength = strlen(data_name);

		if(mz_fwrite_u64(out, datanamelength) != 0){
			goto error;
		}

		if(fwrite(data_name, sizeof(char), datanamelength, out) != datanamelength){
			goto error;
		}

		uint64_t datasize = mz->data_sizes[i];

		if(mz_fwrite_u64(out, datasize) != 0){
			goto error;
		}

		if (fwrite(mz->data[i], 1, datasize, out) != datasize) {
			goto error;
		}
	}
	
	mz->creation_time = (uint64_t)time(NULL);
	if(mz_fwrite_u64(out, mz->creation_time) != 0){
		goto error;
	}
	
	fclose(out);
	return 0;

error:
    if(out){
        fclose(out);
        remove(archive);
    }
	return -1;
}

void mz_memsave_close(MZ_MEMSAVE *mz)
{
    if (!mz)
        return;

    for (uint64_t i = 0; i < mz->data_count; i++) {
		free(mz->data_names[i]);
		free(mz->data[i]);
	}

    free(mz->data_sizes);
    free(mz->data_names);
    free(mz->data);
    free(mz);
}
