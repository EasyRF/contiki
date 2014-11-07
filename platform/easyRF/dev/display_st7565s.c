#include "contiki.h"
#include "display_st7565s.h"


/* Update interval */
#define ST7565S_UPDATE_INTERVAL           (CLOCK_SECOND / 10)


#define DISPLAY_CMD   false
#define DISPLAY_DATA  true


#define DISPL_CMD_ONOFF                   0xAE /* b0 = on/off */
#define DISPL_CMD_START_LINE              0x40 /* b0-b5 = line 0-63 */
#define DISPL_CMD_PAGE_ADDR               0xB0
#define DISPL_CMD_COLUMN_ADDR_LOWBYTE     0x00
#define DISPL_CMD_COLUMN_ADDR_HIGHBYTE    0x10
#define DISPL_CMD_ADC_SEL                 0xA0 /* b0 = normal/reverse */
#define DISPL_CMD_NORMAL_REVERSE          0xA6 /* b0 = reverse/normal */
#define DISPL_CMD_ALLPOINTS_ONOFF         0xA4 /* b0 = on/off */
#define DISPL_CMD_BIAS                    0xA2 /* b0 = bias1/bias2 */
#define DISPL_CMD_RESET                   0xE2
#define DISPL_CMD_COMMON_OUT_MODE_SEL     0xC0 /* b3 = normal/reverse, b0-b2 = mode*/
#define DISPL_CMD_POWER                   0x28 /* b0 = voltage follower, b1=voltage regulator, b2=booster */
#define DISPL_CMD_V_RATIO                 0x20 /* b0-b2 = ratio */
#define DISPL_CMD_CONTRAST                0x81 /* DOUBLE BYTE CMD! second byte is contrast 0-63 */
#define DISPL_CMD_CURSOR                  0xAC /* DOUBLE BYTE CMD! b0 = on/off, second byte b0-b1=OFF/1sblink/0.5sblink/continious */

#define LCD_SET_COLUMN(ca)    do{ write_command(DISPL_CMD_COLUMN_ADDR_LOWBYTE|(ca&0x0F));write_command(DISPL_CMD_COLUMN_ADDR_HIGHBYTE|((ca>>4)&0x0F)); }while(0)
#define LCD_SET_PAGE(pa)      write_command(DISPL_CMD_PAGE_ADDR|(pa&0x0F));
#define LCD_SET_POS(ca,pa)    do{ LCD_SET_COLUMN(ca);LCD_SET_PAGE(pa); }while(0)

static int                   refresh_interval;                       /* ms between one and the next refresh */
static bool                  freezed;                                /* freezed the display */
static bool                  needs_update;                           /* Something is changed in the graphical data */
static unsigned char         data[DISPL_LAYER_CNT][DISPL_DATA_SIZE]; /* contains the pixel data. The data is inside the driver struct so the BMP, FONT drivers can rapidly edit the data this way  */
static short                 selected_layer;                         /* current activated layer */
static short                 active_layer;                           /* current displayed layer */
static short                 contrast;
static bool                  backlight_on;

static unsigned char * pdata_start=0;
static unsigned char * pdata_end=0;
static unsigned char * pdata_active_start=0;
static unsigned char * pdata_active_end=0;

/*---------------------------------------------------------------------------*/
PROCESS(st7565s_process, "ST7565s display process");
/*---------------------------------------------------------------------------*/
static void
write_command(uint8_t cmd)
{
  st7565s_arch_spi_select(DISPLAY_CMD);
  st7565s_arch_spi_write(cmd);
  st7565s_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static void
write_data(uint8_t data)
{
  st7565s_arch_spi_select(DISPLAY_DATA);
  st7565s_arch_spi_write(data);
  st7565s_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
static int
set_defaults(void)
{
  active_layer      = 0;
  contrast          = 37;
  backlight_on      = false;
  freezed           = 0;
  refresh_interval  = 100; // ms
  selected_layer    = 0;
  needs_update      = 1;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  write_command(DISPL_CMD_POWER|7); /* Power control */
  write_command(DISPL_CMD_ONOFF|1); /* Enable display */

  process_start(&st7565s_process, 0);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  process_exit(&st7565s_process);

  write_command(DISPL_CMD_POWER|0); /* Power control */
  write_command(DISPL_CMD_ONOFF|0); /* Enable display */
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
freeze(bool freeze)
{
  freezed=freeze;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
clear(void)
{
  memset(data,0,sizeof(data));
  needs_update=1;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
force_update(void)
{
  if(!pdata_active_start)
    return DISPL_ERR;

  if(freezed)
    return 0;

  int i;
  for(i=0;i<DISPL_DATA_SIZE;i++){
    if(i%128==0)
      LCD_SET_POS(4,i>>DISPL_X_SIZE_2LOG); /* First 4 columns are not used by display */
    write_data(pdata_active_start[((i%128)<<(DISPL_Y_SIZE_2LOG-3)) + (i>>DISPL_X_SIZE_2LOG)]);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
layer_select(short layerid)
{
  if(layerid>=DISPL_LAYER_CNT)
    return DISPL_ERR;
  selected_layer= layerid;
  pdata_start           = data[layerid];
  pdata_end                = pdata_start + DISPL_DATA_SIZE;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
layer_activate(short layerid)
{
  if(layerid>=DISPL_LAYER_CNT)
    return DISPL_ERR;
  active_layer      = layerid;
  pdata_active_start           = data[layerid];
  pdata_active_end          = pdata_start + DISPL_DATA_SIZE;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
layer_cpy(short to, short from)
{
  /* Not implemented */
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
set_px(int x, int y, COLOR_t color)
{
  if(!pdata_start)
    return DISPL_ERR;

  unsigned char * pos = (unsigned char *)( (int)pdata_start + ( (x<<(DISPL_Y_SIZE_2LOG-3)) + (y>>3) ) );
  unsigned char mask = 1<<((y)%8);
  if(pos<pdata_end){
    if(color)
      *pos|=mask;
    else
      *pos&=~mask;
  }
  needs_update=1;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
set_contrast(short c)
{
  contrast = c;
  write_command(0x81);
  write_command(contrast);          // Contrast
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
set_backlight(bool on)
{
  backlight_on = on;
  st7565s_arch_set_backlight(on);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  set_defaults();

  st7565s_arch_spi_init();

  freezed=0;

  write_command(DISPL_CMD_BIAS|0);                              /* BIAS RATIO High */
  write_command(DISPL_CMD_ADC_SEL|1);                         /* Normal ADC Segment driver Direction */
  write_command(DISPL_CMD_COMMON_OUT_MODE_SEL|0);          /* Normal Common Output Mode Select */
  write_command(DISPL_CMD_V_RATIO|5);                         /* V0 Voltage Internal Resistor Ratio Set 0-7 */
  // INITIALIZE DDRAM
  //  2 - Display start line set
  //  3 - Page address set
  //  4 - Column address set
  //  6 - Display data write
  //  1 - Display ON/OFF

  clear();
  set_contrast(contrast);
  set_backlight(backlight_on);
  pdata_start           = data[selected_layer];
  pdata_end             = pdata_start + DISPL_DATA_SIZE;
  pdata_active_start    = data[active_layer];
  pdata_active_end      = pdata_start + DISPL_DATA_SIZE;
  force_update();
  return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(st7565s_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, ST7565S_UPDATE_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    force_update();

    etimer_restart(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
const struct display_driver displ_drv_st7565s =
{
  init,
  on,
  off,
  freeze,
  clear,
  force_update,
  layer_select,
  layer_activate,
  layer_cpy,
  set_px,
  set_contrast,
  set_backlight
};
/*---------------------------------------------------------------------------*/
