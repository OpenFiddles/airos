/*
FILE PATH: include/bmp.h
CONTRIBUTOR (Optional): Textile
PURPOSE: Read-only BMP parser for BOOT_LOGO.bmp and icon_menubar.bmp (24 or 32 bpp, uncompressed).
*/
#ifndef AIROS_BMP_H
#define AIROS_BMP_H

#define BMP_OK 0
#define BMP_ERR_SIZE (-1)
#define BMP_ERR_MAGIC (-2)
#define BMP_ERR_FORMAT (-3)

struct bmp_info {
    int width;
    int height;
    int bpp;
    int row_stride;
    int top_down;
    unsigned int pixel_offset;
};

int bmp_parse(const unsigned char *data, unsigned int len, struct bmp_info *out);
int bmp_pixel(const unsigned char *data, unsigned int len, const struct bmp_info *info, int x, int y, unsigned int *rgb);

#endif
