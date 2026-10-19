#include "return_codes.h"
#include "tf_picture.h"
#include "tf_pnm_file_write.h"
#include "tf_return_codes.h"
#include "tf_tiff_file_read.h"

#include <stdio.h>

static int tf_result_to_app_result(TFReturnCode return_code)
{
	switch (return_code)
	{
	case TF_SUCCESS:
	{
		return ERROR_SUCCESS;
	}
	case TF_ERROR_OPEN_FILE:
	{
		return ERROR_CANNOT_OPEN_FILE;
	}
	case TF_MEMORY_ERROR:
	{
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	case TF_DATA_INVALID:
	{
		return ERROR_DATA_INVALID;
	}
	case TF_INPUT_INVALID:
	{
		return ERROR_ARGUMENTS_INVALID;
	}
	case TF_UNSUPPORTED_TYPE:
	{
		return ERROR_UNSUPPORTED;
	}
	case TF_WRITE_ERROR:
	{
		return ERROR_CANNOT_OPEN_FILE;
	}
	default:
	{
		return ERROR_UNKNOWN;
	}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "Wrong amount of arguments.");
		return ERROR_ARGUMENTS_INVALID;
	}
	const char *input_filename = argv[1];
	const char *output_filename = argv[2];
	TF_Picture picture;
	tf_picture_init(&picture);
	TFReturnCode return_code = tf_tiff_read_file(input_filename, &picture);
	if (return_code != TF_SUCCESS)
	{
		tf_picture_free(&picture);
		return tf_result_to_app_result(return_code);
	}

	return_code = tf_pnm_write_to_file(output_filename, &picture);
	tf_picture_free(&picture);
	return tf_result_to_app_result(return_code);
}
