

#include "tf_picture.h"
#include "tf_pnm_file_write.h"
#include "tf_return_codes.h"

#include <stdio.h>

#define GRAYSCALE_FORMAT 1
#define RBG_FORMAT 3

TFReturnCode tf_pnm_write_to_file(const char *output_filename, const TF_Picture *pic)
{
	if (output_filename == NULL || pic == NULL || pic->data == NULL)
	{
		fprintf(stderr, "Something wrong with input.\n");
		return TF_INPUT_INVALID;
	}
	FILE *output_file = fopen(output_filename, "wb");
	if (output_file == NULL)
	{
		fprintf(stderr, "Can`t open output file.\n");
		return TF_ERROR_OPEN_FILE;
	}
	const char *PNM_format = NULL;
	TFReturnCode return_code = TF_SUCCESS;
	switch (pic->channel)
	{
	case GRAYSCALE_FORMAT:
	{
		PNM_format = "P5";
		break;
	}
	case RBG_FORMAT:
	{
		PNM_format = "P6";
		break;
	}
	default:
	{
		fprintf(stderr, "Unsupported amount of channels.\n");
		return_code = TF_UNSUPPORTED_TYPE;
		goto lcleanup;
	}
	}
	if (fprintf(output_file, "%s\n%u %u\n255\n", PNM_format, pic->width, pic->length) < 0)
	{
		fprintf(stderr, "Can`t write header to output file.\n");
		return_code = TF_WRITE_ERROR;
		goto lcleanup;
	}
	size_t data_size = (size_t)pic->width * pic->length * pic->channel;
	if (fwrite(pic->data, sizeof(pic->data[0]), data_size, output_file) != data_size)
	{
		fprintf(stderr, "Can`t write data to output file.\n");
		return_code = TF_WRITE_ERROR;
		goto lcleanup;
	}

	fclose(output_file);
	return TF_SUCCESS;
lcleanup:
	fclose(output_file);
	return return_code;
}

#undef GRAYSCALE_FORMAT
#undef RBG_FORMAT
