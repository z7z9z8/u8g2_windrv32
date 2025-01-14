/*

  u8x8_framebuffer.c
  
  a framebuffer device

*/

#if 0
#include <stdio.h>
#include <string.h>

#include "sys/win32drv/common/win32drv.h"
#include "u8g2.h"		/* because of u8g2_Setup... */

/*========================================================*/
/* framebuffer struct */

struct _u8x8_win32drv_struct
{
	u8x8_msg_cb u8x8_bitmap_display_old_cb;
	int fbfd;
	uint32_t width;
	uint32_t height;
	uint8_t *u8x8_buf;
	uint8_t *u8g2_buf;
	uint8_t *fbp;
	uint32_t active_color;
};

typedef struct _u8x8_win32drv_struct u8x8_win32drv_t;

/*========================================================*/
/* framebuffer functions */

uint8_t u8x8_win32drv_alloc(u8x8_win32drv_t *fb)
{
	size_t tile_width;
	size_t tile_height;
	size_t screensize = 0;

	if ( fb->u8x8_buf != NULL )
		free(fb->u8x8_buf);

	fb->active_color = 0xFFFFFF;
	tile_width = (fb->width+7)/8;
	tile_height = (fb->height+7)/8;
	screensize = tile_width*tile_height * 8;

	/* allocate the tile buffer twice, one for u8x8 and another bitmap for u8g2 */
	fb->u8x8_buf = (uint8_t *)malloc(screensize*2);
	fb->u8g2_buf = (uint8_t *)fb->u8x8_buf + screensize;

	if ( fb->u8x8_buf == NULL ) {
		fb->u8g2_buf = NULL;
		return 0;
	}

	// Map the device to memory
	fb->fbp = 

	memset(fb->fbp,0x00,screensize);
	return 1;
}

void u8x8_win32drv_DrawTiles(u8x8_win32drv_t *fb, uint16_t tx, uint16_t ty, uint8_t tile_cnt, uint8_t *tile_ptr)
{
	uint8_t byte;
	memset(fb->u8x8_buf,0x00,8*tile_cnt);

	for(int i=0; i < tile_cnt * 8; i++){
		byte = *tile_ptr++;
		for(int bit=0; bit < 8;bit++){
			if(byte & (1 << bit))
				fb->u8x8_buf[tile_cnt*bit+(i/8)] |= (1 << i%8);
		}
	}

	switch (fb->vinfo.bits_per_pixel) {
		case 1:
			memcpy(fb->fbp+ty*8*tile_cnt, fb->u8x8_buf, tile_cnt*8);
			break;
		case 16:{
			uint16_t pixel;
			uint16_t *fbp16 = (uint16_t *)fb->fbp;
			long int location = 0;

			uint8_t b = (fb->active_color & 0x0000FF) >> 0;
			uint8_t g = (fb->active_color & 0x00FF00) >> 8;
			uint8_t r = (fb->active_color & 0xFF0000) >> 16;

			for(int y=0; y<8;y++){
				for(int x=0; x<8*tile_cnt;x++){
					if(fb->u8x8_buf[(x/8) + (y*tile_cnt) ] & (1 << x%8))
						pixel =  r<<11 | g << 5 | b;
					else
						pixel = 0x000000;
					location = (x + fb->vinfo.xoffset) + ((ty*8)+y + fb->vinfo.yoffset) * fb->finfo.line_length / 2;
					memcpy(&fbp16[location], &pixel, sizeof(pixel));
				}
			}
		}break;
		case 32:{
			uint32_t pixel;
			uint32_t *fbp32 = (uint32_t *)fb->fbp;
			long int location = 0;

			for(int y=0; y<8;y++){
				for(int x=0; x<8*tile_cnt;x++){
					if(fb->u8x8_buf[(x/8) + (y*tile_cnt) ] & (1 << x%8))
						pixel = fb->active_color;
					else
						pixel = 0x000000;
					location = (x + fb->vinfo.xoffset) + ((ty*8)+y + fb->vinfo.yoffset) * fb->finfo.line_length / 4;
					memcpy(&fbp32[location], &pixel, sizeof(pixel));
				}
			}
		}break;
		//TODO support 24/16/8 bits_per_pixel
		default:
			break;
	}
}

/*========================================================*/
/* global objects for the framebuffer */

static u8x8_win32drv_t u8x8_win32drv;
static u8x8_display_info_t u8x8_libuxfb_info =
{
	/* chip_enable_level = */ 0,
	/* chip_disable_level = */ 1,

	/* post_chip_enable_wait_ns = */ 0,
	/* pre_chip_disable_wait_ns = */ 0,
	/* reset_pulse_width_ms = */ 0,
	/* post_reset_wait_ms = */ 0,
	/* sda_setup_time_ns = */ 0,
	/* sck_pulse_width_ns = */ 0,
	/* sck_clock_hz = */ 4000000UL,
	/* spi_mode = */ 1,
	/* i2c_bus_clock_100kHz = */ 0,
	/* data_setup_time_ns = */ 0,
	/* write_pulse_width_ns = */ 0,
	/* tile_width = */ 8,		/* dummy value */
	/* tile_hight = */ 4,		/* dummy value */
	/* default_x_offset = */ 0,
	/* flipmode_x_offset = */ 0,
	/* pixel_width = */ 64,		/* dummy value */
	/* pixel_height = */ 32		/* dummy value */
};


/*========================================================*/
/* functions for handling of the global objects */

/* allocate bitmap */
/* will be called by u8x8_SetupBitmap or u8g2_SetupBitmap */
static uint8_t u8x8_Setwin32drvDevice(U8X8_UNUSED u8x8_t *u8x8, int fbfd)
{
	struct fb_var_screeninfo *vinfo = &u8x8_win32drv.vinfo;

	u8x8_win32drv.width = 296;
	u8x8_win32drv.height = 152;

	/* update the global framebuffer object, allocate memory */
	if ( u8x8_win32drv_alloc(&u8x8_win32drv) == 0 )
		return 0;

	u8g2_win32_init(hInstance, SW_SHOWNORMAL, 296, 152, NULL);

	/* update the u8x8 info object */
	u8x8_libuxfb_info.tile_width = (u8x8_win32drv.width+7)/8;
	u8x8_libuxfb_info.tile_height = (u8x8_win32drv.height+7)/8;
	u8x8_libuxfb_info.pixel_width = u8x8_win32drv.width;
	u8x8_libuxfb_info.pixel_height = u8x8_win32drv.height;
	return 1;
}

/* draw tiles to the bitmap, called by the device procedure */
static void u8x8_Drawwin32drvTiles(U8X8_UNUSED u8x8_t *u8x8, uint16_t tx, uint16_t ty, uint8_t tile_cnt, uint8_t *tile_ptr)
{
	u8x8_win32drv_DrawTiles(&u8x8_win32drv, tx, ty, tile_cnt, tile_ptr);
}

/*========================================================*/

static uint8_t u8x8_framebuffer_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	u8g2_uint_t x, y, c;
	uint8_t *ptr;
	switch(msg)
	{
	case U8X8_MSG_DISPLAY_SETUP_MEMORY:
		u8x8_d_helper_display_setup_memory(u8x8, &u8x8_libuxfb_info);
		break;
	case U8X8_MSG_DISPLAY_INIT:
		u8x8_d_helper_display_init(u8x8);	/* update low level interfaces (not required here) */
		break;
	case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
		break;
	case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
		break;
	case U8X8_MSG_DISPLAY_DRAW_TILE:
		x = ((u8x8_tile_t *)arg_ptr)->x_pos;
		y = ((u8x8_tile_t *)arg_ptr)->y_pos;
		c = ((u8x8_tile_t *)arg_ptr)->cnt;
		ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
		do
		{
			u8x8_Drawwin32drvTiles(u8x8, x, y, c, ptr);
			x += c;
			arg_int--;
		} while( arg_int > 0 );
		break;
	default:
	  return 0;
	}
	return 1;
}


/*========================================================*/
/* u8x8 and u8g2 setup functions */

void u8x8_Setupwin32drv(u8x8_t *u8x8)
{
	u8x8_Setwin32drvDevice(u8x8,fbfd);

	/* setup defaults */
	u8x8_SetupDefaults(u8x8);

	/* setup specific callbacks */
	u8x8->display_cb = u8x8_framebuffer_cb;

	/* setup display info */
	u8x8_SetupMemory(u8x8);
}

void u8g2_Setupwin32drv(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb)
{

	/* allocate bitmap, assign the device callback to u8x8 */
	u8x8_Setupwin32drv(u8g2_GetU8x8(u8g2));

	/* configure u8g2 in full buffer mode */
	u8g2_SetupBuffer(u8g2, u8x8_win32drv.u8g2_buf, (u8x8_libuxfb_info.pixel_height+7)/8, u8g2_ll_hvline_vertical_top_lsb, u8g2_cb);
}

void u8x8_win32drvSetActiveColor(uint32_t color)
{
	u8x8_win32drv.active_color = color;
}
#endif