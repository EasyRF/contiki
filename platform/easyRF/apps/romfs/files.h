#ifndef FILES_H_
#define FILES_H_

#define NR_OF_EMBFILES         7

struct embedded_file{
  const unsigned char * filename;
  const unsigned char * data;
  long size;
};

const struct embedded_file embfile[NR_OF_EMBFILES];

#endif /* FILES_H_ */
