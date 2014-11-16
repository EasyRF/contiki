#ifndef AUTOFS_H
#define AUTOFS_H

#include "cfs.h"


int autofs_open(const char *name, int flags);
void autofs_close(int fd);
cfs_offset_t autofs_seek(int fd, cfs_offset_t offset, int whence);
int autofs_read(int fd, void *buf, unsigned size);

#endif // AUTOFS_H
