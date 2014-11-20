#ifndef BMP_HEADER_H
#define BMP_HEADER_H

struct bmp_header {
  /* Signature */
  unsigned short int signature;

  /* File size in bytes */
  unsigned int file_size;

  /* Reserved fields */
  unsigned short int reserved1;
  unsigned short int reserved2;

  /* Offset to image data, bytes */
  unsigned int offset;

  /* Header size in bytes */
  unsigned int size;

  /* Width of image   */
  int width;

  /* Height of image */
  int height;

  /* Number of colour planes */
  unsigned short int planes;

  /* Bits per pixel */
  unsigned short int bits;

  /* Compression type */
  unsigned int compression;

  /* Image size in bytes */
  unsigned int imagesize;

  /* Pixels per meter */
  int xresolution;

  /* Pixels per meter */
  int yresolution;

  /* Number of colours */
  unsigned int ncolours;

  /* Important colours */
  unsigned int importantcolours;
} __attribute__ ((packed));


#define BMP_SIGNATURE 0x4D42

#endif // BMP_HEADER_H
