#pragma once

#include <stdint.h>

typedef struct TF_Picture
{
	uint32_t width;
	uint32_t length;
	uint16_t channel;
	uint8_t *data;
} TF_Picture;

void tf_picture_free(TF_Picture *pic);
void tf_picture_init(TF_Picture *pic);
