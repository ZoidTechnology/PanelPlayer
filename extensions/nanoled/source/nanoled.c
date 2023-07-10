#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "nanoled.h"

#define PACKET_SIZE 4098
#define BUFFER_SIZE (PACKET_SIZE + PACKET_SIZE / 254 + 2)

struct nanoled {
	int device;
	uint8_t *buffer;
};

static uint8_t insert(uint8_t *buffer, int *block, int *index, uint8_t value)
{
	int distance = *index - *block;

	if (value == 0 || distance == 255)
	{
		buffer[*block] = distance;
		*block = *index;
		(*index)++;
	}

	if (value != 0)
	{
		buffer[(*index)++] = value;
	}
	else if (distance == 255)
	{
		insert(buffer, block, index, value);
	}
	
	return value;
}

nanoled *nanoled_init(char *device)
{
	nanoled *instance;

	if ((instance = malloc(sizeof(*instance))) == NULL)
	{
		perror("Failed to allocate memory for instance");
		return NULL;
	}

	if ((instance->device = open(device, O_RDWR)) == -1)
	{
		perror("Failed to open device");
		goto free_instance;
	}

	struct termios options;

	if (tcgetattr(instance->device, &options) == -1)
	{
		perror("Failed get terminal options");
		goto close_device;
	}

	cfmakeraw(&options);
	options.c_cc[VMIN] = 0;

	if (tcsetattr(instance->device, TCSAFLUSH, &options) == -1)
	{
		perror("Failed set terminal options");
		goto close_device;
	}

	if ((instance->buffer = malloc(BUFFER_SIZE)) == NULL)
	{
		perror("Failed to allocate memory for buffer");
		goto close_device;
	}

	instance->buffer[0] = 0;
	return instance;

close_device:
	close(instance->device);

free_instance:
	free(instance);
	return NULL;
}

int nanoled_write(nanoled *instance, int pin, uint8_t *data, int length)
{
	int block = 1;
	int bufferIndex = 2;
	uint16_t header = (pin << 12) | (length - 1);
	uint8_t checksum = 0;

	checksum ^= insert(instance->buffer, &block, &bufferIndex, header >> 8);
	checksum ^= insert(instance->buffer, &block, &bufferIndex, header);
	
	for (int dataIndex = 0; dataIndex < length; dataIndex++)
	{
		checksum ^= insert(instance->buffer, &block, &bufferIndex, data[dataIndex]);
	}

	insert(instance->buffer, &block, &bufferIndex, 0);
	int number = write(instance->device, instance->buffer, --bufferIndex);

	if (number != bufferIndex)
	{
		if (number == -1)
		{
			perror("Failed to write packet");
		}

		return -1;
	}

	return checksum;
}

int nanoled_read(nanoled *instance)
{
	uint8_t byte;
	int number;
	bool success = false;

	while ((number = read(instance->device, &byte, 1)) == 1)
	{
		success = true;
	}

	if (number == -1)
	{
		perror("Failed to read from device");
		return -1;
	}

	if (success)
	{
		return byte;
	}

	return -1;
}

void nanoled_destroy(nanoled *instance)
{
	free(instance->buffer);
	close(instance->device);
	free(instance);
}