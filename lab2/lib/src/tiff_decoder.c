#include "tiff_decoder.h"

#include "tf_picture.h"
#include "tf_return_codes.h"
#include "tiff_file_read.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static TFReturnCode pic_prepare(const TiffImageData *tiff_data, TF_Picture *pic, size_t *image_size)
{
	*image_size = (size_t)tiff_data->width_256 * tiff_data->length_257 * tiff_data->samples_per_pixel_277;
	pic->data = malloc(*image_size * sizeof(*pic->data));
	if (pic->data == NULL)
	{
		fprintf(stderr, "Memory allocation error.\n");
		return TF_MEMORY_ERROR;
	}
	pic->width = tiff_data->width_256;
	pic->length = tiff_data->length_257;
	pic->channel = tiff_data->samples_per_pixel_277;
	return TF_SUCCESS;
}

static TFReturnCode
	get_strip_data(const uint8_t *input_file_data, size_t input_file_size, uint32_t offset, size_t strip_size, const uint8_t **strip_data)
{
	if (offset > input_file_size || strip_size > input_file_size - offset)
	{
		fprintf(stderr, "Problem with strip.\n");
		return TF_DATA_INVALID;
	}
	*strip_data = input_file_data + offset;
	return TF_SUCCESS;
}

static TFReturnCode zlib_to_tf_return_codes(int zlib_return_code)
{
	switch (zlib_return_code)
	{
	case Z_OK:
	{
		return TF_SUCCESS;
	}
	case Z_BUF_ERROR:
	{
		fprintf(stderr, "Problems with buffer.\n");
		return TF_DATA_INVALID;
	}
	case Z_MEM_ERROR:
	{
		fprintf(stderr, "Problems with memory.\n");
		return TF_MEMORY_ERROR;
	}
	case Z_DATA_ERROR:
	{
		fprintf(stderr, "Problems with compressed data.\n");
		return TF_DATA_INVALID;
	}
	default:
	{
		fprintf(stderr, "Unsupported zlib return code.\n");
		return TF_UNSUPPORTED_TYPE;
	}
	}
}

TFReturnCode tiff_decode_no_compression(const uint8_t *input_file_data, size_t input_file_size, const TiffImageData *tiff_data, TF_Picture *pic)
{
	size_t image_size = 0;
	TFReturnCode return_code = pic_prepare(tiff_data, pic, &image_size);
	if (return_code != TF_SUCCESS)
	{
		return return_code;
	}

	size_t written_bytes = 0;
	for (uint32_t i = 0; i < tiff_data->strip_offsets_count_273; i++)
	{
		size_t strip_size = tiff_data->strip_byte_counts_279[i];
		const uint8_t *strip_data = NULL;
		if (written_bytes + strip_size > image_size)
		{
			fprintf(stderr, "Some problems with strip size.\n");
			return_code = TF_DATA_INVALID;
			goto lcleanup_no_compression;
		}
		return_code = get_strip_data(input_file_data, input_file_size, tiff_data->strip_offsets_273[i], strip_size, &strip_data);
		if (return_code != TF_SUCCESS)
		{
			goto lcleanup_no_compression;
		}
		memcpy(pic->data + written_bytes, strip_data, strip_size);
		written_bytes = written_bytes + strip_size;
	}
	if (written_bytes != image_size)
	{
		fprintf(stderr, "Some problems with image size.\n");
		return_code = TF_DATA_INVALID;
		goto lcleanup_no_compression;
	}
	return TF_SUCCESS;

lcleanup_no_compression:
	tf_picture_free(pic);
	return return_code;
}

TFReturnCode tiff_decode_deflate(const uint8_t *input_file_data, size_t input_file_size, const TiffImageData *tiff_data, TF_Picture *pic)
{
	size_t image_size = 0;
	TFReturnCode return_code = pic_prepare(tiff_data, pic, &image_size);
	if (return_code != TF_SUCCESS)
	{
		return return_code;
	}
	size_t written_bytes = 0;
	for (uint32_t i = 0; i < tiff_data->strip_offsets_count_273; i++)
	{
		size_t strip_size = tiff_data->strip_byte_counts_279[i];
		const uint8_t *strip_data = NULL;

		return_code = get_strip_data(input_file_data, input_file_size, tiff_data->strip_offsets_273[i], strip_size, &strip_data);
		if (return_code != TF_SUCCESS)
		{
			goto lcleanup_deflate;
		}

		uLongf destLen = image_size - written_bytes;
		int uncompress_result = uncompress(pic->data + written_bytes, &destLen, strip_data, strip_size);
		return_code = zlib_to_tf_return_codes(uncompress_result);
		if (return_code != TF_SUCCESS)
		{
			goto lcleanup_deflate;
		}
		written_bytes = written_bytes + destLen;
	}
	if (written_bytes != image_size)
	{
		fprintf(stderr, "Some problems with image size.\n");
		return_code = TF_DATA_INVALID;
		goto lcleanup_deflate;
	}
	return TF_SUCCESS;
lcleanup_deflate:
	tf_picture_free(pic);
	return return_code;
}

TFReturnCode tiff_decode_packbits(const uint8_t *input_file_data, size_t input_file_size, const TiffImageData *tiff_data, TF_Picture *pic)
{
	size_t image_size = 0;
	TFReturnCode return_code = pic_prepare(tiff_data, pic, &image_size);
	if (return_code != TF_SUCCESS)
	{
		return return_code;
	}
	size_t written_bytes = 0;
	for (uint32_t i = 0; i < tiff_data->strip_offsets_count_273; i++)
	{
		size_t strip_size = tiff_data->strip_byte_counts_279[i];
		const uint8_t *strip_data = NULL;
		return_code = get_strip_data(input_file_data, input_file_size, tiff_data->strip_offsets_273[i], strip_size, &strip_data);
		if (return_code != TF_SUCCESS)
		{
			goto lcleanup_packbits;
		}
		size_t pos = 0;
		while (pos < strip_size)
		{
			int8_t header = strip_data[pos++];
			if (header >= 0)
			{
				size_t bytes_amount = header + 1;
				if (pos + bytes_amount > strip_size || written_bytes + bytes_amount > image_size)
				{
					fprintf(stderr, "Problems with PackBits size.\n");
					return_code = TF_DATA_INVALID;
					goto lcleanup_packbits;
				}
				for (size_t j = 0; j < bytes_amount; j++)
				{
					pic->data[written_bytes++] = strip_data[pos++];
				}
			}
			else if (header == -128)
			{
				continue;
			}
			else
			{
				size_t bytes_amount = 1 - header;
				if (pos >= strip_size || written_bytes + bytes_amount > image_size)
				{
					fprintf(stderr, "Problems with PackBits size.\n");
					return_code = TF_DATA_INVALID;
					goto lcleanup_packbits;
				}
				uint8_t byte_to_copy = strip_data[pos++];
				for (size_t times = 0; times < bytes_amount; times++)
				{
					pic->data[written_bytes++] = byte_to_copy;
				}
			}
		}
	}
	if (written_bytes != image_size)
	{
		fprintf(stderr, "Some problems with image size.\n");
		return_code = TF_DATA_INVALID;
		goto lcleanup_packbits;
	}
	return TF_SUCCESS;
lcleanup_packbits:
	tf_picture_free(pic);
	return return_code;
}
