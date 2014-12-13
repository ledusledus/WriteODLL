/*
 *    Copyright 2012 Peter Curtis
 *
 *    This file is part of libocad.
 *
 *    libocad is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    libocad is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with libocad.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "libocad.h"

/** Structure for compacting entity indexes - used in the compaction callback functions.
 */
typedef struct _IndexBuilder {
	u8 *p;			// Pointer where we can start writing data
	u8 *base;		// Pointer to base of memory block (base - p is used for offsets)
	dword *pfirst;	// Pointer to location of first index block offset
	void *idx;		// Pointer to the current index block, starts out as NULL
	int i;			// Index of the last object written to the index block, starts out as 0
} IndexBuilder;

/** Copies src to dest, advances the destination pointer to a dword alignment, and returns
 *  the new value of the destination pointer.
 */
static u8 *copy_and_advance(u8 *dest, const void *src, u32 size) {
	u64 tmp;
	u8 *end;
	if (src != NULL) memcpy(dest, src, size);
	// align to dword
	tmp = (u64)dest + size + (4 - 1);
	tmp -= tmp % 4;
	end = (u8 *)tmp;
	if (src == NULL) size = 0;
	memset(dest + size, 0, (end - (dest + size)));
	return (u8 *)tmp;
}

static bool ocad_file_compact_symbol_cb(void *param, OCADFile *pfile, OCADSymbol *symbol) {
	OCADSymbolEntry *entry;
	IndexBuilder *b = (IndexBuilder *)param;
	OCADSymbolIndex *idx = (OCADSymbolIndex *)b->idx;
	if (++b->i == 256) {
		u8 *next = b->p;
		b->p = copy_and_advance(b->p, NULL, sizeof(OCADSymbolIndex));
		if (idx) idx->next = (next - b->base); else *(b->pfirst) = (next - b->base);
		b->idx = next; b->i = 0;
		idx = (OCADSymbolIndex *)b->idx;
	}
	entry = &(idx->entry[b->i]);

	entry->ptr = (b->p - b->base);

	b->p = copy_and_advance(b->p, symbol, symbol->size);
	return TRUE;
}

static bool ocad_file_compact_object_entry_cb(void *param, OCADFile *pfile, OCADObjectEntry *entry) {
	OCADObjectEntry *enew;
	OCADObject *object;
	IndexBuilder *b = (IndexBuilder *)param;
	OCADObjectIndex *idx = (OCADObjectIndex *)b->idx;
	if (++b->i == 256) {
		u8 *next = b->p;
		b->p = copy_and_advance(b->p, NULL, sizeof(OCADObjectIndex));
		if (idx) idx->next = (next - b->base); else *(b->pfirst) = (next - b->base);
		b->idx = next; b->i = 0;
		idx = (OCADObjectIndex *)b->idx;
	}
	enew = &(idx->entry[b->i]);
	object = ocad_object(pfile, entry);

	enew->ptr = (b->p - b->base);
	enew->npts = object->npts + object->ntext;
	ocad_object_entry_refresh(pfile, enew, object);

	b->p = copy_and_advance(b->p, object, ocad_object_size(object));
	return TRUE;
}

static bool ocad_file_compact_string_entry_cb(void *param, OCADFile *pfile, OCADStringEntry *entry) {
	IndexBuilder *b = (IndexBuilder *)param;
	OCADStringIndex *idx = (OCADStringIndex *)b->idx;
	OCADStringEntry *enew;
	OCADCString *str;
	if (++b->i == 256) {
		u8 *next = b->p;
		b->p = copy_and_advance(b->p, NULL, sizeof(OCADStringIndex));
		if (idx) idx->next = (next - b->base); else *(b->pfirst) = (next - b->base);
		b->idx = next; b->i = 0;
		idx = (OCADStringIndex *)b->idx;
	}
	enew = &(idx->entry[b->i]);
	str = ocad_string(pfile, entry);

	// Write a new index entry
	enew->ptr = (b->p - b->base);
	enew->size = entry->size;
	enew->type = entry->type;

	b->p = copy_and_advance(b->p, str, entry->size);
	return TRUE;
}


int ocad_init() {
	// Do some assertions to make sure stuff is packed properly
	if (	sizeof(OCADFileHeader) != 0x48
		||	sizeof(OCADColor) != 0x48
	){
		fprintf(stderr, "*** WARNING: structure packing check failed. libocad was probably not compiled correctly.***\n");
		fprintf(stderr, "magic       = %02lx\n", offsetof(OCADFileHeader, magic));
        fprintf(stderr, "ftype       = %02lx\n", offsetof(OCADFileHeader, ftype));
		fprintf(stderr, "major       = %02lx\n", offsetof(OCADFileHeader, major));
		fprintf(stderr, "minor       = %02lx\n", offsetof(OCADFileHeader, minor));
		fprintf(stderr, "osymidx     = %02lx\n", offsetof(OCADFileHeader, osymidx));
		fprintf(stderr, "oobjidx     = %02lx\n", offsetof(OCADFileHeader, oobjidx));
		fprintf(stderr, "osetup      = %02lx\n", offsetof(OCADFileHeader, osetup));
		fprintf(stderr, "ssetup      = %02lx\n", offsetof(OCADFileHeader, ssetup));
		fprintf(stderr, "Sizeof char is %lu\n", sizeof(char));
		fprintf(stderr, "Sizeof short is %lu\n", sizeof(short));
		fprintf(stderr, "Sizeof int is %lu\n", sizeof(int));
		fprintf(stderr, "Sizeof long is %lu\n", sizeof(long));
		fprintf(stderr, "Sizeof long long is %lu\n", sizeof(long long));
		exit(-10);
	}
	return 0;
}

int ocad_shutdown() {
	return 0;
}

int ocad_file_open(OCADFile **pfile, const char *filename) {
	struct stat fs;
	int left;
	int err = 0;
	u8 *p;
	dword offs;
	OCADFile *file = *pfile;
	if (file == NULL) {
		file = (OCADFile *)malloc(sizeof(OCADFile));
		if (file == NULL) return -1;
	}
	memset(file, 0, sizeof(OCADFile));
	file->filename = (const char *)my_strdup(filename);
	file->fd = _open(file->filename, O_RDONLY | O_BINARY);
	if (file->fd <= 0) { err = -2; goto ocad_file_open_1; }
	if (fstat(file->fd, &fs) < 0) { err = -3; goto ocad_file_open_1; }
	file->size = fs.st_size;
	file->reserved_size = file->size;

#ifdef MMAP_AVAILABLE
	file->buffer = mmap(NULL, file->size, PROT_READ | PROT_WRITE, 0, file->fd, 0);
	if (file->buffer == NULL || file->buffer == MAP_FAILED) { err = -4; goto ocad_file_open_1; }
#else
	file->mapped = FALSE;
	file->buffer = (u8*)malloc(file->size);
	if (file->buffer == NULL) { err = -1; goto ocad_file_open_1; }
	left = file->size;
	p = file->buffer;
	while (left > 0) {
		int got = _read(file->fd, p, left);
		if (got <= 0) { fprintf(stderr,"got=%d sz=%d\n",got,file->size);err = -4; goto ocad_file_open_1; }
		p += got; left -= got;
	}
#endif

	file->header = (OCADFileHeader *)file->buffer;
	file->colors = (OCADColor *)(file->buffer + 0x48);
	offs = file->header->osetup;
	if (offs > 0) file->setup = (OCADSetup *)(file->buffer + offs);

	*pfile = file;
	goto ocad_file_open_0;

ocad_file_open_1:
	ocad_file_close(file);
ocad_file_open_0:
	return err;
}

int ocad_file_open_mapped(OCADFile **pfile, const char *filename) {
	return -10;
}

int ocad_file_open_memory(OCADFile **pfile, u8* buffer, u32 size) {
	dword offs;
	
	OCADFile *file = *pfile;
	if (file == NULL) {
		file = (OCADFile *)malloc(sizeof(OCADFile));
		if (file == NULL) return -1;
	}
	memset(file, 0, sizeof(OCADFile));
	
	file->mapped = FALSE;
	file->size = size;
	file->buffer = buffer;
	if (file->buffer == NULL) { return -1; }
	
	file->header = (OCADFileHeader *)file->buffer;
	file->colors = (OCADColor *)(file->buffer + 0x48);
	offs = file->header->osetup;
	if (offs > 0) file->setup = (OCADSetup *)(file->buffer + offs);
	
	*pfile = file;
	return 0;
}

int ocad_file_close(OCADFile *pfile) {
#ifdef MMAP_AVAILABLE
	if (pfile->buffer) munmap(pfile->buffer, pfile->size);
#else
	if (pfile->buffer) free(pfile->buffer);
#endif
	if (pfile->fd) _close(pfile->fd);
	if (pfile->filename) free((void *)pfile->filename);
	return 0;
}

int ocad_file_save(OCADFile *pfile) {
	if (pfile->mapped) {
		// FIXME: sync the memory map
		return -10;
	}
	return ocad_file_save_as(pfile, pfile->filename);
}

int ocad_file_save_as(OCADFile *pfile, const char *filename) {
	// This behaves the same whether or not the file is memory mapped
	// It saves to another file, without modifying the filename
	int err = 0;
	int got;
	int fd = _open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0664);
	if (fd < 0) { err = -2; goto ocad_file_save_as_0; }
	got = _write(fd, pfile->buffer, pfile->size);
	if (got != pfile->size) { err = -3; goto ocad_file_save_as_1; }

ocad_file_save_as_1:
	_close(fd);
ocad_file_save_as_0:
	return err;
}

int ocad_file_new(OCADFile **pfile) {
	OCADFile *pnew;
	u8 *dest, *p;
	u32 size;
	
	// Create OCADFile struct
	pnew = (OCADFile *)malloc(sizeof(OCADFile));
	if (pnew == NULL) return -1;
	pnew->filename = NULL;
	pnew->fd = 0;
	pnew->mapped = FALSE;
	
	// Allocate buffer
	size = 1024 * 1024;	// start with 1 MiB
	dest = (u8 *)malloc(size);
	if (dest == NULL) return -1;
	memset(dest, 0, size);
	p = dest;
	
	pnew->buffer = dest;
	pnew->reserved_size = size;
	
	// Place header at start
	pnew->header = (OCADFileHeader *)dest;
	p += sizeof(OCADFileHeader);

	// Color table
	pnew->colors = (OCADColor *)p;
	p += 256 * sizeof(OCADColor);
	p += 32 * sizeof(OCADColorSeparation);
	
	// Setup data
	pnew->header->osetup = (p - dest);
	pnew->header->ssetup = sizeof(OCADSetup);	// NOTE: This does not include all possible elements!
	pnew->setup = (OCADSetup *)p;
	p += pnew->header->ssetup;
	
	// First symbol block
	pnew->header->osymidx = (p - dest);
	p += sizeof(OCADSymbolIndex);
	
	// First object block
	pnew->header->oobjidx = (p - dest);
	p += sizeof(OCADObjectIndex);
	
	// String block
	pnew->header->ostringidx = (p - dest);
	p += sizeof(OCADStringIndex);
	
	// Done
	pnew->size = (p - dest);
	*pfile = pnew;
	return 0;
}

int ocad_file_reserve(OCADFile *file, int amount) {
	u32 old_reserved_size = file->reserved_size;
	if (file->reserved_size - file->size >= amount)
		return 0;
	
	u32 header_offset = (u8*)file->header - file->buffer;
	u32 colors_offset = (u8*)file->colors - file->buffer;
	u32 setup_offset = (u8*)file->setup - file->buffer;
	
	while (file->reserved_size - file->size < amount) {
		file->reserved_size *= 2;
	}
	file->buffer = (u8*)realloc(file->buffer, file->reserved_size);
	
	file->header = (OCADFileHeader*)(file->buffer + header_offset);
	file->colors = (OCADColor*)(file->buffer + colors_offset);
	file->setup = (OCADSetup*)(file->buffer + setup_offset);
	
	if (file->buffer == NULL) return -1;
	memset(file->buffer + old_reserved_size, 0, file->reserved_size - old_reserved_size);
	return 0;
}

int ocad_file_compact(OCADFile *pfile) {
	OCADFile dfile, *pnew;
	u8 *dest, *p;
	u32 size;
	IndexBuilder b;
	if (!pfile || !pfile->header) return -1; // invalid file

	// compact should always make the file smaller, so the current size should be enough?
	pnew = &dfile;
	dest = (u8 *)malloc(pfile->size);
	p = dest;
	pnew->buffer = dest;
	pnew->size = pfile->size; // will change this later...
	pnew->reserved_size = pfile->size;
	pnew->header = (OCADFileHeader *)dest;

	b.base = dest;
	fprintf(stderr, "Compacting: base=%p\n", dest);

	// Copy the file header
	p = copy_and_advance(p, pfile->header, sizeof(OCADFileHeader));
	fprintf(stderr, "Compacting: copied file header, p=%p\n", p);

	// Copy the color table
	pnew->colors = (OCADColor *)p;
	size = pfile->header->ncolors * sizeof(OCADColor);
	p = copy_and_advance(p, pfile->colors, size);
	fprintf(stderr, "Compacting: copied color table, p=%p\n", p);

	// Copy the setup data
	size = pfile->header->ssetup;
	pnew->header->osetup = (p - dest);
	pnew->header->ssetup = size;
	pnew->setup = (OCADSetup *)p;
	p = copy_and_advance(p, pfile->setup, size);
	fprintf(stderr, "Compacting: copied setup block, p=%p\n", p);

	// Copy the symbols
	b.p = p; b.pfirst = &(pnew->header->osymidx); b.idx = NULL; b.i = 255;
	ocad_symbol_iterate(pfile, ocad_file_compact_symbol_cb, &b);
	fprintf(stderr, "Compacting: copied symbols, p=%p\n", b.p);
	//dump_bytes(dest, b.p - dest);

	// Copy the objects
	b.pfirst = &(pnew->header->oobjidx); b.idx = NULL; b.i = 255;
	ocad_object_entry_iterate(pfile, ocad_file_compact_object_entry_cb, &b);
	fprintf(stderr, "Compacting: copied objects, p=%p\n", b.p);

	// Copy the strings
	b.pfirst = &(pnew->header->ostringidx); b.idx = NULL; b.i = 255;
	ocad_string_entry_iterate(pfile, ocad_file_compact_string_entry_cb, &b);
	fprintf(stderr, "Compacting: copied template, p=%p\n", b.p);

	// We're done!
	p = b.p;
	pnew->size = (p - dest);
	fprintf(stderr, "Compaction changed size from %x to %x\n", pfile->size, pnew->size);
	free(pfile->buffer);
	pfile->buffer = pnew->buffer;
	pfile->size = pnew->size;
	pfile->header = pnew->header;
	pfile->colors = pnew->colors;
	pfile->setup = pnew->setup;

	return 0;
}

int ocad_export(OCADFile *pfile, void *opts) {
	OCADExportOptions *options = (OCADExportOptions *)opts;
    return options->do_export(pfile, options);
}

int ocad_export_file(OCADFile *pfile, const char *filename, void *opts) {
	int ret;
	OCADExportOptions *options = (OCADExportOptions *)opts;
	FILE *file = fopen(filename, "wb");
	if (file == NULL) return -2;
	options->output = file;
	ret = ocad_export(pfile, options);
	fclose(file);
	return ret;
}



typedef struct _ocad_file_bounds_data {
	bool empty;
	OCADRect rect;
} ocad_file_bounds_data;

static void ocad_file_bounds_union(ocad_file_bounds_data *d, const OCADRect *r) {
	if (d->empty) { 
		memcpy(&(d->rect), r, sizeof(OCADRect)); 
		d->empty = FALSE; 
	}
	else {
		ocad_rect_union(&(d->rect), r);
	}
}

static bool ocad_file_bounds_cb(void *param, OCADFile *file, OCADObjectEntry *entry) {
	if (entry && entry->symbol && entry->ptr) {
		ocad_file_bounds_union((ocad_file_bounds_data *)param, &(entry->rect));
	}
	return TRUE;
}

bool ocad_file_bounds(OCADFile *file, OCADRect *rect) {
	ocad_file_bounds_data data;
	data.empty = TRUE;
	ocad_object_entry_iterate(file, ocad_file_bounds_cb, &data);
	if (!data.empty) {
		memcpy(rect, &(data.rect), sizeof(OCADRect));
		return TRUE;
	}
	return FALSE;
}
