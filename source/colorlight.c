#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "colorlight.h"

#define MAX_PIXELS 497

struct colorlight {
	int socket;
	struct msghdr message;
};

static uint8_t frameHeader[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x22, 0x22, 0x33, 0x44, 0x55, 0x66};

colorlight *colorlight_init(char *interface)
{
	colorlight *instance;

	if ((instance = calloc(1, sizeof(*instance))) == NULL)
	{
		perror("Failed to allocate memory for instance");
		return NULL;
	}

	if ((instance->socket = socket(AF_PACKET, SOCK_RAW, 0)) == -1)
	{
		perror("Failed to create socket");
		goto free_instance;
	}

	struct ifreq request;
	memset(&request, 0, sizeof(request));
	strcpy(request.ifr_name, interface);

	if (ioctl(instance->socket, SIOCGIFINDEX, &request) == -1)
	{
		perror("Failed to find interface");
		goto close_socket;
	}

	struct sockaddr_ll *address;

	if ((address = calloc(1, sizeof(*address))) == NULL)
	{
		perror("Failed to allocate memory for socket address");
		goto close_socket;
	}

	address->sll_ifindex = request.ifr_ifindex;
	address->sll_halen = ETH_ALEN;

	instance->message.msg_name = address;
	instance->message.msg_namelen = sizeof(*address);

	struct iovec *data;

	if ((data = malloc(sizeof(*data) * 3)) == NULL)
	{
		perror("Failed to allocate memory for message data vector");
		goto free_address;
	}

	data[0].iov_base = frameHeader;
	data[0].iov_len = sizeof(frameHeader);

	instance->message.msg_iov = data;

	return instance;

free_address:
	free(address);

close_socket:
	close(instance->socket);

free_instance:
	free(instance);
	return NULL;
}

void colorlight_send_row(colorlight *instance, uint16_t row, uint16_t width, uint8_t *data)
{
	instance->message.msg_iovlen = 3;

	uint8_t header[] = {0x55, row >> 8, row, 0x00, 0x00, 0x00, 0x00, 0x08, 0x88};

	instance->message.msg_iov[1].iov_base = header;
	instance->message.msg_iov[1].iov_len = sizeof(header);

	for (uint16_t offset = 0; offset < width; offset += MAX_PIXELS)
	{
		header[3] = offset >> 8;
		header[4] = offset;

		uint16_t pixels = width - offset;

		if (pixels > MAX_PIXELS)
		{
			pixels = MAX_PIXELS;
		}

		header[5] = pixels >> 8;
		header[6] = pixels;

		instance->message.msg_iov[2].iov_base = data + offset * 3;
		instance->message.msg_iov[2].iov_len = pixels * 3;

		if (sendmsg(instance->socket, &(instance->message), 0) == -1)
		{
			perror("Failed to send row data packet");
		}
	}
}

void colorlight_send_update(colorlight *instance, uint8_t red, uint8_t green, uint8_t blue)
{
	instance->message.msg_iovlen = 2;

	uint8_t packet[100] = {[0] = 0x01, [1] = 0x07, [24] = 0x05};

	packet[23] = (red > green) ? ((red > blue) ? red : blue) : ((green > blue) ? green : blue);
	packet[26] = red;
	packet[27] = green;
	packet[28] = blue;

	instance->message.msg_iov[1].iov_base = packet;
	instance->message.msg_iov[1].iov_len = sizeof(packet);

	if (sendmsg(instance->socket, &(instance->message), 0) == -1)
	{
		perror("Failed to send update packet");
	}
}

void colorlight_send_brightness(colorlight *instance, uint8_t red, uint8_t green, uint8_t blue)
{
	instance->message.msg_iovlen = 2;

	uint8_t packet[65] = {[0] = 0x0A, [4] = 0xFF};

	packet[1] = red;
	packet[2] = green;
	packet[3] = blue;

	instance->message.msg_iov[1].iov_base = packet;
	instance->message.msg_iov[1].iov_len = sizeof(packet);

	if (sendmsg(instance->socket, &(instance->message), 0) == -1)
	{
		perror("Failed to send brightness packet");
	}
}

void colorlight_destroy(colorlight *instance)
{
	close(instance->socket);
	free(instance->message.msg_iov);
	free(instance->message.msg_name);
	free(instance);
}