#pragma once

#include "tf_picture.h"
#include "tf_return_codes.h"

TFReturnCode tf_tiff_read_file(const char *input_filename, TF_Picture *pic);
