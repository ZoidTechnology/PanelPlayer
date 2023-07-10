#ifndef NANOLED_H
#define NANOLED_H

#include <stdint.h>

typedef struct nanoled nanoled;

nanoled *nanoled_init(char *device);
int nanoled_write(nanoled *instance, int pin, uint8_t *data, int length);
int nanoled_read(nanoled *instance);
void nanoled_destroy(nanoled *instance);

#endif