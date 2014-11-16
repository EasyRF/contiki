/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 EasyRF
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "files.h"
#include "romfs.h"
#include "log.h"


#define ROMFS_FD_SET_SIZE     8

#define ROMFS_FD_FREE         0x0
#define ROMFS_FD_READ         0x1


/* File descriptor macros. */
#define FD_VALID(fd)					\
  ((fd) >= 0 && (fd) < ROMFS_FD_SET_SIZE && 	\
  romfs_fd_set[(fd)].flags != ROMFS_FD_FREE)


/* The file descriptor structure. */
struct file_desc {
  cfs_offset_t offset;
  const struct embedded_file * file;
  uint8_t flags;
};

static struct file_desc romfs_fd_set[ROMFS_FD_SET_SIZE];

/*---------------------------------------------------------------------------*/
static int
get_available_fd(void)
{
  int i;

  for(i = 0; i < ROMFS_FD_SET_SIZE; i++) {
    if(romfs_fd_set[i].flags == ROMFS_FD_FREE) {
      return i;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
static const struct embedded_file *
find_file(const char *name)
{
  int i;

  /* Search file in embedded file array */
  for(i = 0; i < NR_OF_EMBFILES; i++) {
    TRACE("%s == %s", embfile[i].filename, name);
    if (strcmp((const char *)embfile[i].filename, name) == 0) {
      return &embfile[i];
    }
  }

  WARN("File not found");

  return NULL;
}
/*---------------------------------------------------------------------------*/
int
romfs_open(const char *name, int flags)
{
  int fd;
  struct file_desc *fdp;

  /* Ignore flags, this is a read-only fs */
  (void) flags;

  fd = get_available_fd();
  if(fd < 0) {
    WARN("ROMFS: Failed to allocate a new file descriptor!");
    return -1;
  }

  fdp = &romfs_fd_set[fd];
  fdp->flags = 0;

  fdp->file = find_file(name);
  if(fdp->file == NULL) {
    WARN("ROMFS: File not found!");
    return -1;
  }

  fdp->flags = ROMFS_FD_READ;
  fdp->offset = 0;

  return fd;
}
/*---------------------------------------------------------------------------*/
void
romfs_close(int fd)
{
  if (FD_VALID(fd)) {
    romfs_fd_set[fd].flags = ROMFS_FD_FREE;
    romfs_fd_set[fd].file = NULL;
  }
}
/*---------------------------------------------------------------------------*/
cfs_offset_t
romfs_seek(int fd, cfs_offset_t offset, int whence)
{
  struct file_desc *fdp;
  cfs_offset_t new_offset;

  if(!FD_VALID(fd)) {
    return -1;
  }
  fdp = &romfs_fd_set[fd];

  if(whence == CFS_SEEK_SET) {
    new_offset = offset;
  } else if(whence == CFS_SEEK_END) {
    new_offset = fdp->file->size + offset;
  } else if(whence == CFS_SEEK_CUR) {
    new_offset = fdp->offset + offset;
  } else {
    return (cfs_offset_t)-1;
  }

  if(new_offset < 0 || new_offset > fdp->file->size) {
    WARN("Incorrect offset %d", new_offset);
    return -1;
  }

  return fdp->offset = new_offset;
}
/*---------------------------------------------------------------------------*/
int
romfs_read(int fd, void *buf, unsigned size)
{
  struct file_desc *fdp;
  const struct embedded_file *file;

  if (!FD_VALID(fd)) {
    return -1;
  }

  fdp = &romfs_fd_set[fd];
  file = fdp->file;
  if(fdp->offset + size > file->size) {
    size = file->size - fdp->offset;
  }

  memcpy(buf, fdp->file->data + fdp->offset, size);

  fdp->offset += size;

  return size;
}
/*---------------------------------------------------------------------------*/
