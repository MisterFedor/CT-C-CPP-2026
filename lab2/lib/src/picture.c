#include "tf_picture.h"

#include <stdlib.h>

void tf_picture_init(TF_Picture *pic)
{
	if (pic == NULL)
	{
		return;
	}
	pic->channel = 0;
	pic->width = 0;
	pic->length = 0;
	pic->data = NULL;
}
void tf_picture_free(TF_Picture *pic)
{
	if (pic == NULL)
	{
		return;
	}
	free(pic->data);
	pic->channel = 0;
	pic->width = 0;
	pic->length = 0;
	pic->data = NULL;
}
