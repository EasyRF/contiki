#include <stdlib.h>
#include "contiki.h"
#include "flash.h"
#include "display_st7565s.h"
#include "cfs-coffee.h"
#include "romfs.h"
#include "canvas.h"
#include "canvas_text.h"
#include "log.h"


/*---------------------------------------------------------------------------*/
PROCESS(display_process, "DISPLAY Process");
AUTOSTART_PROCESSES(&display_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(display_process, ev, data)
{
  static char buf[128];
  cfs_offset_t filesize, read, pos;

  PROCESS_BEGIN();

  INFO("DISPLAY test started");

  display_st7565s.on();
  display_st7565s.set_value(DISPLAY_BACKLIGHT, 1);
  display_st7565s.set_value(DISPLAY_FLIP_X, 1);
  display_st7565s.set_value(DISPLAY_FLIP_Y, 1);

  display_value_t width, height;
  display_st7565s.get_value(DISPLAY_WIDTH, &width);
  display_st7565s.get_value(DISPLAY_HEIGHT, &height);

  /* Open external flash */
  EXTERNAL_FLASH.open();

#if 1
  /* Format cfs */
  cfs_coffee_format();

  /* Open file for writing in CFS */
  int cfs_fd = cfs_open("test.bmp", CFS_WRITE);

  /* Opend file for reading in romfs */
  int rom_fd = romfs_open("/verdane8_bold.bmp", CFS_READ);

  /* Determine file size */
  filesize = romfs_seek(rom_fd, 0, CFS_SEEK_END) - 1;

  /* Restore offset to start of file */
  romfs_seek(rom_fd, 0, CFS_SEEK_SET);

  /* Copy file data from romfs to cfs in chunks of 128 bytes */
  for (pos = 0; pos < filesize; pos += read) {
    read = romfs_read(rom_fd, buf, sizeof(buf));
    cfs_write(cfs_fd, buf, read);
  }

  /* Close both files */
  cfs_close(cfs_fd);
  romfs_close(rom_fd);
#endif

  canvas_line(&display_st7565s, 0, 0, width, height, 1);
  canvas_line(&display_st7565s, width, 0, 0, height, 1);
  canvas_fill(&display_st7565s, 10, 10, 4, 4, 1);
  canvas_bmp(&display_st7565s, "/logo_easyrf.bmp", 0, 0, 1);

  canvas_text_init(&display_st7565s,"test.bmp", width / 2, height - 20, width / 2, 20, 1, 0);
  canvas_puts("Hello World!");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
