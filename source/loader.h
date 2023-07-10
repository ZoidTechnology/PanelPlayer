#ifndef LOADER_H
#define LOADER_H

#include <stdbool.h>

typedef struct loader loader;

loader *loader_init(int length);
bool loader_add(loader *instance, char *path);
void *loader_get(loader *instance, int *size);
void loader_destroy(loader *instance);

#endif