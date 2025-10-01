#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct decoder decoder;

typedef struct decoder_info
{
	int frame_count;
	int canvas_width;
	int canvas_height;
} decoder_info;

decoder *decoder_init(void *data, int size);
bool decoder_get_info(decoder *instance, decoder_info *info);
bool decoder_has_more_frames(decoder *instance);
bool decoder_get_next(decoder *instance, uint8_t **frame, int *timestamp);
void decoder_destroy(decoder *instance);

#endif
