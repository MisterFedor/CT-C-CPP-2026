
#include "tiff_file_read.h"

#include "tf_picture.h"
#include "tf_return_codes.h"
#include "tf_tiff_file_read.h"
#include "tiff_decoder.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_TO_UINT16_LE(bytes) ((uint32_t)(bytes)[0] | ((uint32_t)(bytes)[1] << 8))
#define BYTES_TO_UINT16_BE(bytes) (((uint32_t)(bytes)[0] << 8) | (uint32_t)(bytes)[1])
#define BYTES_TO_UINT32_LE(bytes)                                                                                      \
	((uint32_t)(bytes)[0] | ((uint32_t)(bytes)[1] << 8) | ((uint32_t)(bytes)[2] << 16) | ((uint32_t)(bytes)[3] << 24))
#define BYTES_TO_UINT32_BE(bytes)                                                                                      \
	(((uint32_t)(bytes)[0] << 24) | ((uint32_t)(bytes)[1] << 16) | ((uint32_t)(bytes)[2] << 8) | (uint32_t)(bytes)[3])

#define INIT_CAPACITY 4096
#define CAPACITY_EXTEND 2
#define BUFFER_SIZE 4096
#define UINT16_MASK 0xFFFFu
#define SHORT_BITS 16
#define EMPTY_VALUES_NUMBER 0
#define SINGLE_VALUES_NUMBER 1
#define NO_COMPRESSION 1
#define DEFLATE_COMPRESSION_FIRST 8
#define DEFLATE_COMPRESSION_SECOND 32946
#define PACKBITS_COMPRESSION 32773
#define SAMPLES_PER_PIXEL_GRAY 1
#define SAMPLES_PER_PIXEL_RGB 3
#define PHOTOMETRIC_WHITE_IS_ZERO 0
#define PHOTOMETRIC_BLACK_IS_ZERO 1
#define PHOTOMETRIC_RGB 2
#define SUPPORTED_BITS_PER_SAMPLE 8
#define HEADER_SIZE 8
#define FORMAT_IDENTIFIER_OFFSET 2
#define FIRST_IFD_OFFSET 4
#define BYTE_ORDER_SIZE 2
#define LITTLE_ENDIAN "II"
#define BIG_ENDIAN "MM"
#define TIFF_IDENTIFIER_NUMBER 42
#define MIN_FIRST_IFD_OFFSET 8
#define IFD_ENTRY_SIZE 12
#define FIELD_TYPE_OFFSET 2
#define VALUES_NUMBER_OFFSET 4
#define VALUES_OR_OFFSET_OFFSET 8
#define IFD_ENTRY_COUNT_SIZE 2
#define TIFF_SHORT_SIZE 2
#define TIFF_LONG_SIZE 4
#define NO_PREDICTOR 1
#define PREDICTOR_HORIZONTAL_DIFFERENCING 2

void tiff_header_init(TiffHeader *header)
{
	if (header == NULL)
	{
		return;
	}
	header->is_LE = false;
	header->format_identifier = 0;
	header->IFD_offset = 0;
}

void tiff_IFDEntry_init(TiffIFDEntry *entry)
{
	if (entry == NULL)
	{
		return;
	}
	entry->tag = 0;
	entry->field_type = 0;
	entry->values_number = 0;
	entry->values_or_offset = 0;
}

void tiff_TiffImageData_init(TiffImageData *tiff_data)
{
	if (tiff_data == NULL)
	{
		return;
	}

	tiff_data->width_256 = 0;
	tiff_data->length_257 = 0;
	tiff_data->bits_per_sample_258 = NULL;
	tiff_data->bits_per_sample_count_258 = 0;
	tiff_data->compression_259 = 1;
	tiff_data->photometric_262 = 0;
	tiff_data->samples_per_pixel_277 = 0;
	tiff_data->rows_per_strip_278 = 0;
	tiff_data->strip_offsets_273 = NULL;
	tiff_data->strip_offsets_count_273 = 0;
	tiff_data->strip_byte_counts_279 = NULL;
	tiff_data->strip_byte_counts_count_279 = 0;
	tiff_data->visited_tags = (TiffVisitedTags){ false };
	tiff_data->predictor_317 = 1;
}

void tiff_TiffImageData_free(TiffImageData *tiff_data)
{
	if (tiff_data == NULL)
	{
		return;
	}
	tiff_data->width_256 = 0;
	tiff_data->length_257 = 0;
	free(tiff_data->bits_per_sample_258);
	tiff_data->bits_per_sample_258 = NULL;
	tiff_data->bits_per_sample_count_258 = 0;
	tiff_data->compression_259 = 1;
	tiff_data->photometric_262 = 0;
	tiff_data->samples_per_pixel_277 = 0;
	tiff_data->rows_per_strip_278 = 0;
	free(tiff_data->strip_offsets_273);
	tiff_data->strip_offsets_273 = NULL;
	tiff_data->strip_offsets_count_273 = 0;
	free(tiff_data->strip_byte_counts_279);
	tiff_data->strip_byte_counts_279 = NULL;
	tiff_data->strip_byte_counts_count_279 = 0;
	tiff_data->visited_tags = (TiffVisitedTags){ false };
	tiff_data->predictor_317 = 1;
}

static TFReturnCode horizontal_differencing(const TiffImageData *tiff_data, TF_Picture *pic)
{
	if (tiff_data == NULL || pic == NULL)
	{
		fprintf(stderr, "predictor problem");
		return TF_INPUT_INVALID;
	}
	if (tiff_data->predictor_317 == NO_PREDICTOR)
	{
		return TF_SUCCESS;
	}
	if (tiff_data->predictor_317 != PREDICTOR_HORIZONTAL_DIFFERENCING)
	{
		return TF_UNSUPPORTED_TYPE;
	}
	uint16_t channels = tiff_data->samples_per_pixel_277;
	size_t row_size = (size_t)tiff_data->width_256 * channels;

	for (uint32_t row = 0; row < tiff_data->length_257; row++)
	{
		uint8_t *row_array = pic->data + row * row_size;

		for (size_t i = channels; i < row_size; i++)
		{
			row_array[i] = row_array[i] + row_array[i - channels];
		}
	}
	return TF_SUCCESS;
}

static void invert_picture(const TiffImageData *tiff_data, TF_Picture *pic)
{
	if (tiff_data->samples_per_pixel_277 != 1 || tiff_data->photometric_262 != 0)
	{
		return;
	}
	size_t image_size = pic->width * pic->length;
	for (size_t i = 0; i < image_size; i++)
	{
		pic->data[i] = ~pic->data[i];
	}
}

static TFReturnCode ensure_capacity(uint8_t **data, size_t cur_size, size_t *cur_capacity, size_t needed_capacity)
{
	if (data == NULL || cur_capacity == NULL)
	{
		fprintf(stderr, "Problems with capacity.\n");
		return TF_INPUT_INVALID;
	}
	if (needed_capacity <= *cur_capacity)
	{
		return TF_SUCCESS;
	}

	size_t ext_capacity = *cur_capacity;
	if (ext_capacity == 0)
	{
		ext_capacity = INIT_CAPACITY;
	}
	while (ext_capacity < needed_capacity)
	{
		ext_capacity *= CAPACITY_EXTEND;
	}
	uint8_t *new_data = malloc(ext_capacity * sizeof(uint8_t));
	if (new_data == NULL)
	{
		fprintf(stderr, "Memory allocation error.\n");
		return TF_MEMORY_ERROR;
	}
	if (cur_size > 0)
	{
		memcpy(new_data, *data, cur_size * sizeof(uint8_t));
	}
	free(*data);
	*data = new_data;
	*cur_capacity = ext_capacity;
	return TF_SUCCESS;
}
static TFReturnCode read_file_to_array(FILE *input_file, uint8_t **data, size_t *data_size)
{
	*data = NULL;
	*data_size = 0;
	size_t cur_capacity = 0;
	uint8_t buffer[BUFFER_SIZE];
	TFReturnCode return_code = TF_SUCCESS;
	while (true)
	{
		size_t readed_bytes = fread(buffer, sizeof(buffer[0]), sizeof(buffer), input_file);
		if (readed_bytes > 0)
		{
			return_code = ensure_capacity(data, *data_size, &cur_capacity, *data_size + readed_bytes);
			if (return_code != TF_SUCCESS)
			{
				goto lcleanup_read_file_to_array;
			}
			memcpy(*data + *data_size, buffer, readed_bytes);
			*data_size = *data_size + readed_bytes;
		}
		if (readed_bytes < BUFFER_SIZE)
		{
			if (!feof(input_file))
			{
				fprintf(stderr, "Problems with read bytes.\n");
				return_code = TF_DATA_INVALID;
				goto lcleanup_read_file_to_array;
			}
			break;
		}
	}
	if (*data_size == 0)
	{
		fprintf(stderr, "Empty file.\n");
		return_code = TF_DATA_INVALID;
		goto lcleanup_read_file_to_array;
	}
	return TF_SUCCESS;

lcleanup_read_file_to_array:
	free(*data);
	*data = NULL;
	*data_size = 0;
	return return_code;
}

static uint32_t get_ifd_value(const TiffIFDEntry *entry, bool is_LE)
{
	if (entry->field_type != SHORT)
	{
		return entry->values_or_offset;
	}
	uint32_t val = entry->values_or_offset;
	if (!is_LE)
	{
		val = val >> SHORT_BITS;
	}
	return val & UINT16_MASK;
}

static TFReturnCode
	read_offset(const uint8_t *input_file_data, size_t input_file_size, const TiffHeader *header, const TiffIFDEntry *entry, uint32_t **out)
{
	if (input_file_data == NULL || input_file_size == 0 || header == NULL || entry == NULL || out == NULL)
	{
		fprintf(stderr, "Problems with input.\n");
		return TF_INPUT_INVALID;
	}
	if (entry->values_number == EMPTY_VALUES_NUMBER)
	{
		fprintf(stderr, "Unexpected situation.\n");
		return TF_DATA_INVALID;
	}
	*out = malloc(entry->values_number * sizeof(uint32_t));
	if (*out == NULL)
	{
		fprintf(stderr, "Memory allocation error.\n");
		return TF_MEMORY_ERROR;
	}

	if (entry->values_number == SINGLE_VALUES_NUMBER)
	{
		(*out)[0] = get_ifd_value(entry, header->is_LE);
		return TF_SUCCESS;
	}
	size_t value_size = 0;
	TFReturnCode return_code = TF_SUCCESS;
	switch (entry->field_type)
	{
	case SHORT:
	{
		value_size = TIFF_SHORT_SIZE;
		break;
	}
	case LONG:
	{
		value_size = TIFF_LONG_SIZE;
		break;
	}
	default:
	{
		fprintf(stderr, "Unsupported field type.\n");
		return_code = TF_UNSUPPORTED_TYPE;
		goto lcleanup_read_offset;
	}
	}
	if (entry->values_or_offset > input_file_size || entry->values_number * value_size > input_file_size - entry->values_or_offset)
	{
		fprintf(stderr, "Problems with offset.\n");
		return_code = TF_DATA_INVALID;
		goto lcleanup_read_offset;
	}

	const uint8_t *offset_data = input_file_data + entry->values_or_offset;
	for (uint32_t i = 0; i < entry->values_number; i++)
	{
		const uint8_t *readed_data = offset_data + i * value_size;
		switch (entry->field_type)
		{
		case SHORT:
		{
			(*out)[i] = header->is_LE ? BYTES_TO_UINT16_LE(readed_data) : BYTES_TO_UINT16_BE(readed_data);
			break;
		}
		case LONG:
		{
			(*out)[i] = header->is_LE ? BYTES_TO_UINT32_LE(readed_data) : BYTES_TO_UINT32_BE(readed_data);
			break;
		}
		default:
		{
			fprintf(stderr, "Unsupported field type.\n");
			return_code = TF_UNSUPPORTED_TYPE;
			goto lcleanup_read_offset;
		}
		}
	}
	return TF_SUCCESS;
lcleanup_read_offset:
	free(*out);
	*out = NULL;
	return return_code;
}

static TFReturnCode
	tiff_decode(const uint8_t *input_file_data, size_t input_file_size, const TiffImageData *tiff_data, TF_Picture *pic)
{
	TFReturnCode return_code = TF_SUCCESS;
	switch (tiff_data->compression_259)
	{
	case NO_COMPRESSION:
	{
		return_code = tiff_decode_no_compression(input_file_data, input_file_size, tiff_data, pic);
		break;
	}
	case DEFLATE_COMPRESSION_FIRST:
	case DEFLATE_COMPRESSION_SECOND:
	{
		return_code = tiff_decode_deflate(input_file_data, input_file_size, tiff_data, pic);
		break;
	}
	case PACKBITS_COMPRESSION:
	{
		return_code = tiff_decode_packbits(input_file_data, input_file_size, tiff_data, pic);
		break;
	}
	default:
	{
		fprintf(stderr, "Unsupported type of TIFF compression.\n");
		return TF_UNSUPPORTED_TYPE;
	}
	}
	if (return_code != TF_SUCCESS)
	{
		return return_code;
	}
	return_code = horizontal_differencing(tiff_data, pic);
	if (return_code != TF_SUCCESS)
	{
		return return_code;
	}
	invert_picture(tiff_data, pic);
	return TF_SUCCESS;
}

static TFReturnCode check_tiff_image_data(const TiffImageData *tiff_data)
{
	if (tiff_data == NULL)
	{
		fprintf(stderr, "Incorrect input.\n");
		return TF_INPUT_INVALID;
	}
	if (!tiff_data->visited_tags.has_width_256 || !tiff_data->visited_tags.has_length_257 ||
		!tiff_data->visited_tags.has_bits_per_sample_258 || !tiff_data->visited_tags.has_photometric_262 ||
		!tiff_data->visited_tags.has_strip_offsets_273 || !tiff_data->visited_tags.has_strip_byte_counts_279)
	{
		fprintf(stderr, "Don`t have required TIFF tags.\n");
		return TF_DATA_INVALID;
	}
	if (tiff_data->width_256 == 0 || tiff_data->length_257 == 0)
	{
		fprintf(stderr, "Incorrect TIFF width or length size.\n");
		return TF_DATA_INVALID;
	}
	switch (tiff_data->samples_per_pixel_277)
	{
	case SAMPLES_PER_PIXEL_GRAY:
	{
		if (tiff_data->photometric_262 != PHOTOMETRIC_WHITE_IS_ZERO && tiff_data->photometric_262 != PHOTOMETRIC_BLACK_IS_ZERO)
		{
			fprintf(stderr, "Incorrect TIFF grayscale image.\n");
			return TF_DATA_INVALID;
		}
		break;
	}
	case SAMPLES_PER_PIXEL_RGB:
	{
		if (tiff_data->photometric_262 != PHOTOMETRIC_RGB)
		{
			fprintf(stderr, "Incorrect TIFF RGB image.\n");
			return TF_DATA_INVALID;
		}
		break;
	}
	default:
	{
		fprintf(stderr, "Unsupported TIFF samples per pixel.\n");
		return TF_UNSUPPORTED_TYPE;
	}
	}

	if (tiff_data->bits_per_sample_258 == NULL || tiff_data->bits_per_sample_count_258 != tiff_data->samples_per_pixel_277)
	{
		fprintf(stderr, "Incorrect TIFF bits per sample.\n");
		return TF_DATA_INVALID;
	}
	for (uint32_t i = 0; i < tiff_data->bits_per_sample_count_258; i++)
	{
		if (tiff_data->bits_per_sample_258[i] != SUPPORTED_BITS_PER_SAMPLE)
		{
			fprintf(stderr, "Unsupported TIFF bits per sample.\n");
			return TF_UNSUPPORTED_TYPE;
		}
	}
	switch (tiff_data->compression_259)
	{
	case NO_COMPRESSION:
	case DEFLATE_COMPRESSION_FIRST:
	case DEFLATE_COMPRESSION_SECOND:
	case PACKBITS_COMPRESSION:
	{
		break;
	}
	default:
	{
		fprintf(stderr, "Unsupported TIFF compression.\n");
		return TF_UNSUPPORTED_TYPE;
	}
	}
	if (tiff_data->strip_byte_counts_279 == NULL || tiff_data->strip_offsets_273 == NULL)
	{
		fprintf(stderr, "Incorrect TIFF strips.\n");
		return TF_DATA_INVALID;
	}
	if (tiff_data->strip_byte_counts_count_279 == 0 || tiff_data->strip_offsets_count_273 == 0 ||
		tiff_data->strip_byte_counts_count_279 != tiff_data->strip_offsets_count_273)
	{
		fprintf(stderr, "Incorrect TIFF strips.\n");
		return TF_DATA_INVALID;
	}
	if (tiff_data->rows_per_strip_278 == 0)
	{
		fprintf(stderr, "Incorrect TIFF rows per strip.\n");
		return TF_DATA_INVALID;
	}
	return TF_SUCCESS;
}

static TFReturnCode tiff_header_parse(const uint8_t *input_file_data, size_t input_file_size, TiffHeader *tiff_header)
{
	if (input_file_data == NULL || input_file_size == 0)
	{
		fprintf(stderr, "Problems with input.\n");
		return TF_INPUT_INVALID;
	}
	if (input_file_size < HEADER_SIZE)
	{
		fprintf(stderr, "Problems with input file size.\n");
		return TF_DATA_INVALID;
	}
	const uint8_t *format = input_file_data;
	const uint8_t *format_identifier = input_file_data + FORMAT_IDENTIFIER_OFFSET;
	const uint8_t *offset = input_file_data + FIRST_IFD_OFFSET;

	if (memcmp(format, LITTLE_ENDIAN, BYTE_ORDER_SIZE) == 0)
	{
		tiff_header->is_LE = true;
		tiff_header->format_identifier = (uint16_t)BYTES_TO_UINT16_LE(format_identifier);
		tiff_header->IFD_offset = BYTES_TO_UINT32_LE(offset);
	}
	else if (memcmp(format, BIG_ENDIAN, BYTE_ORDER_SIZE) == 0)
	{
		tiff_header->is_LE = false;
		tiff_header->format_identifier = (uint16_t)BYTES_TO_UINT16_BE(format_identifier);
		tiff_header->IFD_offset = BYTES_TO_UINT32_BE(offset);
	}
	else
	{
		fprintf(stderr, "Illegal type of TIFF format.\n");
		return TF_DATA_INVALID;
	}
	if (tiff_header->format_identifier != TIFF_IDENTIFIER_NUMBER)
	{
		fprintf(stderr, "Incorrect TIFF format identifier.\n");
		return TF_DATA_INVALID;
	}
	if (tiff_header->IFD_offset < MIN_FIRST_IFD_OFFSET || tiff_header->IFD_offset >= input_file_size)
	{
		fprintf(stderr, "Incorrect TIFF IFD_offset.\n");
		return TF_DATA_INVALID;
	}

	return TF_SUCCESS;
}

static TFReturnCode tiff_IFDEntry_parse(const uint8_t *input_file_data, size_t input_file_size, TiffIFDEntry *entry, bool is_LE)
{
	if (input_file_data == NULL || input_file_size == 0)
	{
		fprintf(stderr, "Problems with input.\n");
		return TF_INPUT_INVALID;
	}
	if (input_file_size < IFD_ENTRY_SIZE)
	{
		fprintf(stderr, "Problems with entry size.\n");
		return TF_DATA_INVALID;
	}
	tiff_IFDEntry_init(entry);
	const uint8_t *tag_bytes = input_file_data;
	const uint8_t *field_bytes = input_file_data + FIELD_TYPE_OFFSET;
	const uint8_t *values_number_bytes = input_file_data + VALUES_NUMBER_OFFSET;
	const uint8_t *values_offset_bytes = input_file_data + VALUES_OR_OFFSET_OFFSET;
	if (is_LE)
	{
		entry->tag = (uint16_t)BYTES_TO_UINT16_LE(tag_bytes);
		entry->field_type = (uint16_t)BYTES_TO_UINT16_LE(field_bytes);
		entry->values_number = BYTES_TO_UINT32_LE(values_number_bytes);
		entry->values_or_offset = BYTES_TO_UINT32_LE(values_offset_bytes);
	}
	else
	{
		entry->tag = (uint16_t)BYTES_TO_UINT16_BE(tag_bytes);
		entry->field_type = (uint16_t)BYTES_TO_UINT16_BE(field_bytes);
		entry->values_number = BYTES_TO_UINT32_BE(values_number_bytes);
		entry->values_or_offset = BYTES_TO_UINT32_BE(values_offset_bytes);
	}
	return TF_SUCCESS;
}

static TFReturnCode
	tiff_IFD_parse(const uint8_t *input_file_data, size_t input_file_size, const TiffHeader *tiff_header, TiffImageData *tiff_data)
{
	if (input_file_data == NULL || input_file_size == 0 || tiff_data == NULL)
	{
		fprintf(stderr, "Problems with input.\n");
		return TF_INPUT_INVALID;
	}
	if (tiff_header->IFD_offset + IFD_ENTRY_COUNT_SIZE > input_file_size)
	{
		fprintf(stderr, "Some problems with entries amount bytes in IFD.\n");
		return TF_DATA_INVALID;
	}
	const uint8_t *ifd_data = input_file_data + tiff_header->IFD_offset;

	uint16_t count_entries = 0;
	if (tiff_header->is_LE)
	{
		count_entries = (uint16_t)BYTES_TO_UINT16_LE(ifd_data);
	}
	else
	{
		count_entries = (uint16_t)BYTES_TO_UINT16_BE(ifd_data);
	}
	const uint8_t *entry_data = ifd_data + IFD_ENTRY_COUNT_SIZE;
	if (count_entries * IFD_ENTRY_SIZE > input_file_size - (tiff_header->IFD_offset + IFD_ENTRY_COUNT_SIZE) ||
		tiff_header->IFD_offset + 2 > input_file_size)
	{
		fprintf(stderr, "Problems with IFD.\n");
		return TF_DATA_INVALID;
	}
	for (uint16_t i = 0; i < count_entries; i++)
	{
		TiffIFDEntry entry;
		TFReturnCode return_code =
			tiff_IFDEntry_parse(entry_data + i * IFD_ENTRY_SIZE, IFD_ENTRY_SIZE, &entry, tiff_header->is_LE);
		if (return_code != TF_SUCCESS)
		{
			return return_code;
		}
		switch (entry.tag)
		{
		case TIFF_TAG_ImageWidth:
		{
			if (tiff_data->visited_tags.has_width_256)
			{
				fprintf(stderr, "Extra width tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.values_number != SINGLE_VALUES_NUMBER)
			{
				fprintf(stderr, "Some problems with ImageWidth values number.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT && entry.field_type != LONG)
			{
				fprintf(stderr, "Some problems with ImageWidth field type.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->width_256 = get_ifd_value(&entry, tiff_header->is_LE);
			tiff_data->visited_tags.has_width_256 = true;
			break;
		}
		case TIFF_TAG_ImageLength:
		{
			if (tiff_data->visited_tags.has_length_257)
			{
				fprintf(stderr, "Extra length tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.values_number != SINGLE_VALUES_NUMBER)
			{
				fprintf(stderr, "Some problems with ImageLength values number.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT && entry.field_type != LONG)
			{
				fprintf(stderr, "Some problems with ImageLength field type.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->length_257 = get_ifd_value(&entry, tiff_header->is_LE);
			tiff_data->visited_tags.has_length_257 = true;
			break;
		}
		case TIFF_TAG_BitsPerSample:
		{
			if (tiff_data->visited_tags.has_bits_per_sample_258)
			{
				fprintf(stderr, "Extra bits per sample tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT)
			{
				fprintf(stderr, "Some problems with BitsPerSample field type.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->bits_per_sample_count_258 = entry.values_number;
			return_code = read_offset(input_file_data, input_file_size, tiff_header, &entry, &tiff_data->bits_per_sample_258);
			if (return_code != TF_SUCCESS)
			{
				return return_code;
			}
			tiff_data->visited_tags.has_bits_per_sample_258 = true;
			break;
		}
		case TIFF_TAG_Compression:
		{
			if (tiff_data->visited_tags.has_compression_259)
			{
				fprintf(stderr, "Extra compression tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.values_number != SINGLE_VALUES_NUMBER)
			{
				fprintf(stderr, "Some problems with Compression values number.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT)
			{
				fprintf(stderr, "Some problems with Compression values number.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->compression_259 = get_ifd_value(&entry, tiff_header->is_LE);
			tiff_data->visited_tags.has_compression_259 = true;
			break;
		}
		case TIFF_TAG_PhotometricInterpretation:
		{
			if (tiff_data->visited_tags.has_photometric_262)
			{
				fprintf(stderr, "Extra photometric tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.values_number != SINGLE_VALUES_NUMBER)
			{
				fprintf(stderr, "Some problems with Photometric Interpretation values number.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT)
			{
				fprintf(stderr, "Some problems with Photometric Interpretation field type.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->photometric_262 = get_ifd_value(&entry, tiff_header->is_LE);
			tiff_data->visited_tags.has_photometric_262 = true;
			break;
		}
		case TIFF_TAG_StripOffsets:
		{
			if (tiff_data->visited_tags.has_strip_offsets_273)
			{
				fprintf(stderr, "Extra strip offsets tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT && entry.field_type != LONG)
			{
				fprintf(stderr, "Some problems with StripOffsets field type.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->strip_offsets_count_273 = entry.values_number;
			return_code = read_offset(input_file_data, input_file_size, tiff_header, &entry, &tiff_data->strip_offsets_273);
			if (return_code != TF_SUCCESS)
			{
				return return_code;
			}
			tiff_data->visited_tags.has_strip_offsets_273 = true;
			break;
		}
		case TIFF_TAG_SamplesPerPixel:
		{
			if (tiff_data->visited_tags.has_samples_per_pixel_277)
			{
				fprintf(stderr, "Extra samples per pixel tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.values_number != SINGLE_VALUES_NUMBER)
			{
				fprintf(stderr, "Some problems with SamplePerPixel values number.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT)
			{
				fprintf(stderr, "Some problems with SamplePerPixel field type.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->samples_per_pixel_277 = get_ifd_value(&entry, tiff_header->is_LE);
			tiff_data->visited_tags.has_samples_per_pixel_277 = true;
			break;
		}
		case TIFF_TAG_RowsPerStrip:
		{
			if (tiff_data->visited_tags.has_rows_per_strip_278)
			{
				fprintf(stderr, "Extra rows per strip tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.values_number != SINGLE_VALUES_NUMBER)
			{
				fprintf(stderr, "Some problems with RowsPerStrip values number.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT && entry.field_type != LONG)
			{
				fprintf(stderr, "Some problems with RowsPerStrip field type.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->rows_per_strip_278 = get_ifd_value(&entry, tiff_header->is_LE);
			tiff_data->visited_tags.has_rows_per_strip_278 = true;
			break;
		}
		case TIFF_TAG_StripByteCounts:
		{
			if (tiff_data->visited_tags.has_strip_byte_counts_279)
			{
				fprintf(stderr, "Extra strip byte counts tag.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT && entry.field_type != LONG)
			{
				fprintf(stderr, "Some problems with StripByteCounts field type.\n");
				return TF_DATA_INVALID;
			}
			tiff_data->strip_byte_counts_count_279 = entry.values_number;
			return_code = read_offset(input_file_data, input_file_size, tiff_header, &entry, &tiff_data->strip_byte_counts_279);
			if (return_code != TF_SUCCESS)
			{
				return return_code;
			}
			tiff_data->visited_tags.has_strip_byte_counts_279 = true;
			break;
		}
		case TIFF_TAG_Predictor:
		{
			if (entry.values_number != SINGLE_VALUES_NUMBER)
			{
				fprintf(stderr, "Some problems with PRedictor values number.\n");
				return TF_DATA_INVALID;
			}
			if (entry.field_type != SHORT)
			{
				fprintf(stderr, "Some problems with PRedictor field type.");
				return TF_DATA_INVALID;
			}
			tiff_data->predictor_317 = get_ifd_value(&entry, tiff_header->is_LE);
			if (tiff_data->predictor_317 != NO_PREDICTOR && tiff_data->predictor_317 != PREDICTOR_HORIZONTAL_DIFFERENCING)
			{
				fprintf(stderr, "Unsupported TIFF PRedictor.\n");
				return TF_UNSUPPORTED_TYPE;
			}
			break;
		}
		default:
		{
			break;
		}
		}
	}

	return TF_SUCCESS;
}

static TFReturnCode tiff_file_parse(const uint8_t *input_file_data, size_t input_file_size, TF_Picture *pic)
{
	if (input_file_data == NULL || input_file_size == 0 || pic == NULL)
	{
		fprintf(stderr, "Problems with input.\n");
		return TF_INPUT_INVALID;
	}
	tf_picture_init(pic);

	TiffHeader header;
	tiff_header_init(&header);

	TiffImageData tiff_data;
	tiff_TiffImageData_init(&tiff_data);

	TFReturnCode return_code = tiff_header_parse(input_file_data, input_file_size, &header);
	if (return_code != TF_SUCCESS)
	{
		goto lcleanup_tiff_file_parse;
	}

	return_code = tiff_IFD_parse(input_file_data, input_file_size, &header, &tiff_data);
	if (return_code != TF_SUCCESS)
	{
		goto lcleanup_tiff_file_parse;
	}
	if (!tiff_data.visited_tags.has_compression_259)
	{
		tiff_data.compression_259 = NO_COMPRESSION;
	}
	if (!tiff_data.visited_tags.has_samples_per_pixel_277)
	{
		tiff_data.samples_per_pixel_277 = SAMPLES_PER_PIXEL_GRAY;
	}
	if (!tiff_data.visited_tags.has_rows_per_strip_278)
	{
		tiff_data.rows_per_strip_278 = tiff_data.length_257;
	}
	return_code = check_tiff_image_data(&tiff_data);
	if (return_code != TF_SUCCESS)
	{
		goto lcleanup_tiff_file_parse;
	}

	return_code = tiff_decode(input_file_data, input_file_size, &tiff_data, pic);
	if (return_code != TF_SUCCESS)
	{
		goto lcleanup_tiff_file_parse;
	}
	tiff_TiffImageData_free(&tiff_data);
	return TF_SUCCESS;

lcleanup_tiff_file_parse:
	tiff_TiffImageData_free(&tiff_data);
	tf_picture_free(pic);
	return return_code;
}

TFReturnCode tf_tiff_read_file(const char *input_filename, TF_Picture *pic)
{
	if (input_filename == NULL || pic == NULL)
	{
		fprintf(stderr, "Something wrong with input.\n");
		return TF_INPUT_INVALID;
	}
	FILE *input_file = fopen(input_filename, "rb");
	if (input_file == NULL)
	{
		fprintf(stderr, "Can`t open input file.\n");
		return TF_ERROR_OPEN_FILE;
	}

	uint8_t *input_file_data = NULL;
	size_t input_file_size = 0;
	TFReturnCode return_code = read_file_to_array(input_file, &input_file_data, &input_file_size);
	fclose(input_file);
	if (return_code != TF_SUCCESS)
	{
		return return_code;
	}
	return_code = tiff_file_parse(input_file_data, input_file_size, pic);
	free(input_file_data);
	return return_code;
}

#undef BYTES_TO_UINT16_LE
#undef BYTES_TO_UINT16_BE
#undef BYTES_TO_UINT32_LE
#undef BYTES_TO_UINT32_BE

#undef INIT_CAPACITY
#undef CAPACITY_EXTEND
#undef BUFFER_SIZE
#undef UINT16_MASK
#undef SHORT_BITS
#undef EMPTY_VALUES_NUMBER
#undef SINGLE_VALUES_NUMBER
#undef NO_COMPRESSION
#undef DEFLATE_COMPRESSION_FIRST
#undef DEFLATE_COMPRESSION_SECOND
#undef PACKBITS_COMPRESSION
#undef SAMPLES_PER_PIXEL_GRAY
#undef SAMPLES_PER_PIXEL_RGB
#undef PHOTOMETRIC_WHITE_IS_ZERO
#undef PHOTOMETRIC_BLACK_IS_ZERO
#undef PHOTOMETRIC_RGB
#undef SUPPORTED_BITS_PER_SAMPLE
#undef HEADER_SIZE
#undef FORMAT_IDENTIFIER_OFFSET
#undef FIRST_IFD_OFFSET
#undef BYTE_ORDER_SIZE
#undef LITTLE_ENDIAN
#undef BIG_ENDIAN
#undef TIFF_IDENTIFIER_NUMBER
#undef MIN_FIRST_IFD_OFFSET
#undef IFD_ENTRY_SIZE
#undef FIELD_TYPE_OFFSET
#undef VALUES_NUMBER_OFFSET
#undef VALUES_OR_OFFSET_OFFSET
#undef IFD_ENTRY_COUNT_SIZE
#undef TIFF_SHORT_SIZE
#undef TIFF_LONG_SIZE
#undef NO_PREDICTOR
#undef PREDICTOR_HORIZONTAL_DIFFERENCING
