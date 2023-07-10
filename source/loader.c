#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "loader.h"

typedef struct loader_queue_item {
	char *path;
	void *data;
	int size;
} loader_queue_item;

struct loader {
	int length;
	loader_queue_item *queue;
	int head;
	int tail;
	bool destroyed;
	pthread_mutex_t lock;
	pthread_cond_t condition;
	pthread_t thread;
};

static void *loader_process(void *parameter)
{
	loader *instance = parameter;
	pthread_mutex_lock(&instance->lock);

	for (int index = 0; ; index = (index + 1) % instance->length)
	{
		while(instance->destroyed || index == instance->head)
		{
			if (instance->destroyed)
			{
				goto unlock_mutex;
			}

			pthread_cond_wait(&instance->condition, &instance->lock);
		}

		loader_queue_item item = instance->queue[index];
		pthread_mutex_unlock(&instance->lock);
		FILE *file;

		if ((file = fopen(item.path, "r")) == NULL)
		{
			perror("Failed to open file");
			goto update_item;
		}

		fseek(file, 0, SEEK_END);
		item.size = ftell(file);

		if (item.size == -1)
		{
			perror("Failed to get file size");
			goto close_file;
		}

		rewind(file);

		if ((item.data = malloc(item.size)) == NULL)
		{
			perror("Failed to allocate memory for file contents");
			goto close_file;
		}

		if (fread(item.data, 1, item.size, file) != item.size)
		{
			perror("Failed to read file");
			free(item.data);
		}

close_file:
		fclose(file);

update_item:
		if (item.data == NULL || item.size < 0)
		{
			item.data = NULL;
			item.size = 0;
		}

		pthread_mutex_lock(&instance->lock);
		instance->queue[index] = item;
		pthread_cond_broadcast(&instance->condition);
	}

unlock_mutex:
	pthread_mutex_unlock(&instance->lock);
	return NULL;
}

loader *loader_init(int length)
{
	loader *instance;

	if ((instance = calloc(1, sizeof(*instance))) == NULL)
	{
		perror("Failed to allocate memory for instance");
		return NULL;
	}

	instance->length = ++length;

	if ((instance->queue = calloc(length, sizeof(*instance->queue))) == NULL)
	{
		perror("Failed to allocate memory for queue");
		goto free_instance;
	}

	pthread_mutex_init(&instance->lock, NULL);
	pthread_cond_init(&instance->condition, NULL);

	if (pthread_create(&instance->thread, NULL, loader_process, instance))
	{
		puts("Failed to create processing thread!");
		goto free_queue;
	}

	return instance;

free_queue:
	pthread_cond_destroy(&instance->condition);
	pthread_mutex_destroy(&instance->lock);

	free(instance->queue);

free_instance:
	free(instance);
	return NULL;
}

bool loader_add(loader *instance, char *path)
{
	pthread_mutex_lock(&instance->lock);

	bool full = false;
	int next = (instance->head + 1) % instance->length;

	if (next == instance->tail)
	{
		puts("Queue is full!");
		full = true;
		goto unlock;
	}

	loader_queue_item *item = &instance->queue[instance->head];

	item->path = path;
	item->size = -1;

	instance->head = next;

	pthread_cond_broadcast(&instance->condition);

unlock:
	pthread_mutex_unlock(&instance->lock);
	return full;
}

void *loader_get(loader *instance, int *size)
{
	pthread_mutex_lock(&instance->lock);

	void *data = NULL;

	if (instance->head == instance->tail)
	{
		puts("No files in queue!");
		goto unlock;
	}

	loader_queue_item *item = &instance->queue[instance->tail];

	while ((*size = item->size) < 0)
	{
		pthread_cond_wait(&instance->condition, &instance->lock);
	}

	data = item->data;

	instance->tail = (instance->tail + 1) % instance->length;

unlock:
	pthread_mutex_unlock(&instance->lock);
	return data;
}

void loader_destroy(loader *instance)
{
	pthread_mutex_lock(&instance->lock);

	instance->destroyed = true;
	pthread_cond_broadcast(&instance->condition);

	pthread_mutex_unlock(&instance->lock);

	pthread_join(instance->thread, NULL);

	pthread_cond_destroy(&instance->condition);
	pthread_mutex_destroy(&instance->lock);

	while(instance->head != instance->tail)
	{
		loader_queue_item item = instance->queue[instance->tail];

		if (item.size >= 0)
		{
			free(instance->queue[instance->tail].data);
		}

		instance->tail = (instance->tail + 1) % instance->length;
	}

	free(instance->queue);
	free(instance);
}