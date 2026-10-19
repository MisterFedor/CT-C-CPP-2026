#pragma once

#include "tf_picture.h"
#include "tf_return_codes.h"

TFReturnCode tf_pnm_write_to_file(const char *output_filename, const TF_Picture *pic);
