#ifndef ROMFS_H
#define ROMFS_H

/* Borrow flags from CFS */
#include "cfs.h"


int romfs_open(const char *name, int flags);
void romfs_close(int fd);
cfs_offset_t romfs_seek(int fd, cfs_offset_t offset, int whence);
int romfs_read(int fd, void *buf, unsigned size);

#endif // ROMFS_H
