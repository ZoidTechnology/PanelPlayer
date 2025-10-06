#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <webp/demux.h>
#include <jpeglib.h>
#include <png.h>
#include <gif_lib.h>

#include "decoder.h"

typedef enum
{
	FORMAT_UNKNOWN,
	FORMAT_WEBP,
	FORMAT_JPEG,
	FORMAT_PNG,
	FORMAT_GIF,
	FORMAT_BMP
} decoder_format;

typedef struct decoder
{
	decoder_format format;
	void *data;
	int size;

	union
	{
		struct
		{
			WebPData webp_data;
			WebPAnimDecoder *webp_decoder;
		};

		struct
		{
			uint8_t *frame_data;
			int frame_width;
			int frame_height;
			bool frame_consumed;
		};

		struct
		{
			GifFileType *gif_file;
			int gif_current_frame;
			int gif_frame_count;
			uint8_t *gif_frame_data;
			int gif_last_timestamp;
		};
	};
} decoder;

static decoder_format detect_format(void *data, int size)
{
	if (size < 12)
	{
		return FORMAT_UNKNOWN;
	}

	uint8_t *bytes = data;

	if (bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
	    bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P')
	{
		return FORMAT_WEBP;
	}

	if (bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF)
	{
		return FORMAT_JPEG;
	}

	if (bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G')
	{
		return FORMAT_PNG;
	}

	if (bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F')
	{
		return FORMAT_GIF;
	}

	if (bytes[0] == 'B' && bytes[1] == 'M')
	{
		return FORMAT_BMP;
	}

	return FORMAT_UNKNOWN;
}

static int gif_read_func(GifFileType *gif, GifByteType *buf, int size)
{
	decoder *instance = gif->UserData;

	static int position = 0;

	if (position + size > instance->size)
	{
		size = instance->size - position;
	}

	if (size <= 0)
	{
		return 0;
	}

	memcpy(buf, (uint8_t *)instance->data + position, size);
	position += size;

	return size;
}

decoder *decoder_init(void *data, int size)
{
	decoder *instance;

	if ((instance = calloc(1, sizeof(*instance))) == NULL)
	{
		perror("Failed to allocate memory for decoder");
		return NULL;
	}

	instance->data = data;
	instance->size = size;
	instance->format = detect_format(data, size);

	switch (instance->format)
	{
		case FORMAT_WEBP:
			instance->webp_data.bytes = data;
			instance->webp_data.size = size;

			if ((instance->webp_decoder = WebPAnimDecoderNew(&instance->webp_data, NULL)) == NULL)
			{
				goto free_instance;
			}
			break;

		case FORMAT_JPEG:
		{
			struct jpeg_decompress_struct cinfo;
			struct jpeg_error_mgr jerr;

			cinfo.err = jpeg_std_error(&jerr);
			jpeg_create_decompress(&cinfo);
			jpeg_mem_src(&cinfo, data, size);
			jpeg_read_header(&cinfo, TRUE);

			cinfo.out_color_space = JCS_EXT_RGBA;
			jpeg_start_decompress(&cinfo);

			instance->frame_width = cinfo.output_width;
			instance->frame_height = cinfo.output_height;

			if ((instance->frame_data = malloc(instance->frame_width * instance->frame_height * 4)) == NULL)
			{
				jpeg_destroy_decompress(&cinfo);
				goto free_instance;
			}

			int row_stride = cinfo.output_width * 4;

			while (cinfo.output_scanline < cinfo.output_height)
			{
				uint8_t *row = instance->frame_data + cinfo.output_scanline * row_stride;
				jpeg_read_scanlines(&cinfo, &row, 1);
			}

			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			instance->frame_consumed = false;
			break;
		}

		case FORMAT_PNG:
		{
			png_image image;
			memset(&image, 0, sizeof(image));
			image.version = PNG_IMAGE_VERSION;

			if (png_image_begin_read_from_memory(&image, data, size) == 0)
			{
				goto free_instance;
			}

			image.format = PNG_FORMAT_RGBA;

			instance->frame_width = image.width;
			instance->frame_height = image.height;

			if ((instance->frame_data = malloc(PNG_IMAGE_SIZE(image))) == NULL)
			{
				png_image_free(&image);
				goto free_instance;
			}

			if (png_image_finish_read(&image, NULL, instance->frame_data, 0, NULL) == 0)
			{
				free(instance->frame_data);
				png_image_free(&image);
				goto free_instance;
			}

			png_image_free(&image);
			instance->frame_consumed = false;
			break;
		}

		case FORMAT_GIF:
		{
			int error;

			if ((instance->gif_file = DGifOpen(instance, gif_read_func, &error)) == NULL)
			{
				goto free_instance;
			}

			if (DGifSlurp(instance->gif_file) == GIF_ERROR)
			{
				DGifCloseFile(instance->gif_file, NULL);
				goto free_instance;
			}

			instance->gif_frame_count = instance->gif_file->ImageCount;
			instance->gif_current_frame = 0;

			if ((instance->gif_frame_data = malloc(instance->gif_file->SWidth * instance->gif_file->SHeight * 4)) == NULL)
			{
				DGifCloseFile(instance->gif_file, NULL);
				goto free_instance;
			}

			memset(instance->gif_frame_data, 0, instance->gif_file->SWidth * instance->gif_file->SHeight * 4);
			instance->gif_last_timestamp = 0;
			break;
		}

		case FORMAT_BMP:
		{
			uint8_t *bytes = data;

			if (size < 54)
			{
				goto free_instance;
			}

			int offset = bytes[10] | (bytes[11] << 8) | (bytes[12] << 16) | (bytes[13] << 24);
			int header_size = bytes[14] | (bytes[15] << 8) | (bytes[16] << 16) | (bytes[17] << 24);

			if (header_size < 40)
			{
				goto free_instance;
			}

			instance->frame_width = bytes[18] | (bytes[19] << 8) | (bytes[20] << 16) | (bytes[21] << 24);
			instance->frame_height = bytes[22] | (bytes[23] << 8) | (bytes[24] << 16) | (bytes[25] << 24);
			int bits_per_pixel = bytes[28] | (bytes[29] << 8);
			int compression = bytes[30] | (bytes[31] << 8) | (bytes[32] << 16) | (bytes[33] << 24);

			if (compression != 0 || (bits_per_pixel != 24 && bits_per_pixel != 32))
			{
				goto free_instance;
			}

			bool bottom_up = instance->frame_height > 0;
			if (instance->frame_height < 0)
			{
				instance->frame_height = -instance->frame_height;
			}

			if ((instance->frame_data = malloc(instance->frame_width * instance->frame_height * 4)) == NULL)
			{
				goto free_instance;
			}

			int row_size = ((bits_per_pixel * instance->frame_width + 31) / 32) * 4;
			uint8_t *src = bytes + offset;

			for (int y = 0; y < instance->frame_height; y++)
			{
				int dest_y = bottom_up ? (instance->frame_height - 1 - y) : y;
				uint8_t *row = src + y * row_size;

				for (int x = 0; x < instance->frame_width; x++)
				{
					int dest_index = (dest_y * instance->frame_width + x) * 4;
					int src_index = x * (bits_per_pixel / 8);

					instance->frame_data[dest_index + 0] = row[src_index + 2];
					instance->frame_data[dest_index + 1] = row[src_index + 1];
					instance->frame_data[dest_index + 2] = row[src_index + 0];
					instance->frame_data[dest_index + 3] = (bits_per_pixel == 32) ? row[src_index + 3] : 255;
				}
			}

			instance->frame_consumed = false;
			break;
		}

		default:
			goto free_instance;
	}

	return instance;

free_instance:
	free(instance);
	return NULL;
}

bool decoder_get_info(decoder *instance, decoder_info *info)
{
	if (instance == NULL || info == NULL)
	{
		return false;
	}

	switch (instance->format)
	{
		case FORMAT_WEBP:
		{
			WebPAnimInfo webp_info;
			WebPAnimDecoderGetInfo(instance->webp_decoder, &webp_info);

			info->frame_count = webp_info.frame_count;
			info->canvas_width = webp_info.canvas_width;
			info->canvas_height = webp_info.canvas_height;
			break;
		}

		case FORMAT_JPEG:
		case FORMAT_PNG:
		case FORMAT_BMP:
			info->frame_count = 1;
			info->canvas_width = instance->frame_width;
			info->canvas_height = instance->frame_height;
			break;

		case FORMAT_GIF:
			info->frame_count = instance->gif_frame_count;
			info->canvas_width = instance->gif_file->SWidth;
			info->canvas_height = instance->gif_file->SHeight;
			break;

		default:
			return false;
	}

	return true;
}

bool decoder_has_more_frames(decoder *instance)
{
	if (instance == NULL)
	{
		return false;
	}

	switch (instance->format)
	{
		case FORMAT_WEBP:
			return WebPAnimDecoderHasMoreFrames(instance->webp_decoder);

		case FORMAT_JPEG:
		case FORMAT_PNG:
		case FORMAT_BMP:
			return !instance->frame_consumed;

		case FORMAT_GIF:
			return instance->gif_current_frame < instance->gif_frame_count;

		default:
			return false;
	}
}

bool decoder_get_next(decoder *instance, uint8_t **frame, int *timestamp)
{
	if (instance == NULL || frame == NULL || timestamp == NULL)
	{
		return false;
	}

	switch (instance->format)
	{
		case FORMAT_WEBP:
			return WebPAnimDecoderGetNext(instance->webp_decoder, frame, timestamp);

		case FORMAT_JPEG:
		case FORMAT_PNG:
		case FORMAT_BMP:
			if (instance->frame_consumed)
			{
				return false;
			}

			*frame = instance->frame_data;
			*timestamp = 0;
			instance->frame_consumed = true;
			return true;

		case FORMAT_GIF:
		{
			if (instance->gif_current_frame >= instance->gif_frame_count)
			{
				return false;
			}

			SavedImage *image = &instance->gif_file->SavedImages[instance->gif_current_frame];
			ColorMapObject *colormap = image->ImageDesc.ColorMap != NULL ? image->ImageDesc.ColorMap : instance->gif_file->SColorMap;

			if (colormap == NULL)
			{
				return false;
			}

			int left = image->ImageDesc.Left;
			int top = image->ImageDesc.Top;
			int width = image->ImageDesc.Width;
			int height = image->ImageDesc.Height;

			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					int gif_index = y * width + x;
					int frame_index = ((top + y) * instance->gif_file->SWidth + (left + x)) * 4;

					uint8_t color_index = image->RasterBits[gif_index];
					GifColorType color = colormap->Colors[color_index];

					int transparent = -1;
					for (int i = 0; i < image->ExtensionBlockCount; i++)
					{
						if (image->ExtensionBlocks[i].Function == GRAPHICS_EXT_FUNC_CODE)
						{
							uint8_t *ext = image->ExtensionBlocks[i].Bytes;
							if (ext[0] & 0x01)
							{
								transparent = ext[3];
							}
						}
					}

					if (color_index != transparent)
					{
						instance->gif_frame_data[frame_index + 0] = color.Red;
						instance->gif_frame_data[frame_index + 1] = color.Green;
						instance->gif_frame_data[frame_index + 2] = color.Blue;
						instance->gif_frame_data[frame_index + 3] = 255;
					}
				}
			}

			int delay = 100;
			for (int i = 0; i < image->ExtensionBlockCount; i++)
			{
				if (image->ExtensionBlocks[i].Function == GRAPHICS_EXT_FUNC_CODE)
				{
					uint8_t *ext = image->ExtensionBlocks[i].Bytes;
					delay = (ext[2] << 8) | ext[1];
					delay *= 10;
				}
			}

			*frame = instance->gif_frame_data;
			*timestamp = instance->gif_last_timestamp + delay;
			instance->gif_last_timestamp = *timestamp;
			instance->gif_current_frame++;

			return true;
		}

		default:
			return false;
	}
}

void decoder_destroy(decoder *instance)
{
	if (instance == NULL)
	{
		return;
	}

	switch (instance->format)
	{
		case FORMAT_WEBP:
			WebPAnimDecoderDelete(instance->webp_decoder);
			break;

		case FORMAT_JPEG:
		case FORMAT_PNG:
		case FORMAT_BMP:
			free(instance->frame_data);
			break;

		case FORMAT_GIF:
			free(instance->gif_frame_data);
			DGifCloseFile(instance->gif_file, NULL);
			break;

		default:
			break;
	}

	free(instance);
}
