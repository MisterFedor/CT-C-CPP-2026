#pragma once
#include "tf_picture.h"
#include "tf_return_codes.h"
#include "tiff_file_read.h"

#include <stdio.h>

TFReturnCode
	tiff_decode_no_compression(const uint8_t *input_file_data, size_t input_file_size, const TiffImageData *tiff_data, TF_Picture *pic);
TFReturnCode tiff_decode_deflate(const uint8_t *input_file_data, size_t input_file_size, const TiffImageData *tiff_data, TF_Picture *pic);
TFReturnCode tiff_decode_packbits(const uint8_t *input_file_data, size_t input_file_size, const TiffImageData *tiff_data, TF_Picture *pic);
