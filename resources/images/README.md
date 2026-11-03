# Boot and menu bar images

Drop the image files here. The kernel looks for them by exact name.

- BOOT_LOGO.bmp: boot screen logo. Waiting for upload.
- icon_menubar.bmp: menu bar icon strip. Waiting for upload.

Format: BMP, 24 or 32 bits per pixel, uncompressed (BI_RGB). kernel/bmp.c rejects other BMP variants.

Status: the parser exists (include/bmp.h, kernel/bmp.c). Drawing needs the 1024x768 graphics mode, which is not built yet, and bmp.o is not in the Makefile yet.
