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

#include "cfs.h"
#include "romfs.h"
#include "autofs.h"
#include "log.h"


#define AUTOFS_FD_SET_SIZE     8

#define AUTOFS_FD_FREE         0x0
#define AUTOFS_FD_USED         0x1


/* File descriptor macros. */
#define FD_VALID(fd)					\
  ((fd) >= 0 && (fd) < AUTOFS_FD_SET_SIZE && 	\
  autofs_fd_set[(fd)].flags != AUTOFS_FD_FREE)

enum fs_type {
  ROMFS_FILE,
  CFS_FILE
};

/* The file descriptor structure. */
struct file_desc {
  int innerfd;
  enum fs_type type;
  uint8_t flags;
};

static struct file_desc autofs_fd_set[AUTOFS_FD_SET_SIZE];

/*---------------------------------------------------------------------------*/
static int
get_available_fd(void)
{
  int i;

  for(i = 0; i < AUTOFS_FD_SET_SIZE; i++) {
    if(autofs_fd_set[i].flags == AUTOFS_FD_FREE) {
      return i;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int
autofs_open(const char *name, int flags)
{
  int fd, innerfd;
  struct file_desc *fdp;

  /* Ignore flags, this is a read-only fs */
  (void) flags;

  fd = get_available_fd();
  if(fd < 0) {
    WARN("ROMFS: Failed to allocate a new file descriptor!");
    return -1;
  }

  fdp = &autofs_fd_set[fd];

  /* First try romfs */
  innerfd = romfs_open(name, flags);
  if (innerfd >= 0) {
    fdp->type = ROMFS_FILE;
    TRACE("ROMFS opened %s", name);
  } else {
    /* Then try cfs */
    innerfd = cfs_open(name, flags);
    if (innerfd >= 0) {
      fdp->type = CFS_FILE;
      TRACE("CFS opened %s", name);
    } else {
      WARN("File not found in ROMFS and CFS");
      return -1;
    }
  }

  fdp->innerfd = innerfd;
  fdp->flags = AUTOFS_FD_USED;

  return fd;
}
/*---------------------------------------------------------------------------*/
void
autofs_close(int fd)
{
  if (FD_VALID(fd)) {
    if (autofs_fd_set[fd].type == ROMFS_FILE) {
      romfs_close(autofs_fd_set[fd].innerfd);
    } else if (autofs_fd_set[fd].type == CFS_FILE) {
      cfs_close(autofs_fd_set[fd].innerfd);
    } else {
      WARN("Unknown file type");
    }
    autofs_fd_set[fd].flags = AUTOFS_FD_FREE;
  }
}
/*---------------------------------------------------------------------------*/
cfs_offset_t
autofs_seek(int fd, cfs_offset_t offset, int whence)
{
  if (autofs_fd_set[fd].type == ROMFS_FILE) {
    return romfs_seek(autofs_fd_set[fd].innerfd, offset, whence);
  } else if (autofs_fd_set[fd].type == CFS_FILE) {
    return cfs_seek(autofs_fd_set[fd].innerfd, offset, whence);
  } else {
    WARN("Unknown file type");
  }

  return -1;
}
/*---------------------------------------------------------------------------*/
int
autofs_read(int fd, void *buf, unsigned size)
{
  if (autofs_fd_set[fd].type == ROMFS_FILE) {
    return romfs_read(autofs_fd_set[fd].innerfd, buf, size);
  } else if (autofs_fd_set[fd].type == CFS_FILE) {
    return cfs_read(autofs_fd_set[fd].innerfd, buf, size);
  } else {
    WARN("Unknown file type");
  }

  return -1;
}
