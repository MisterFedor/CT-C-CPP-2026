#pragma once
#include <stdbool.h>
#include <stdint.h>

enum TiffFieldType
{
	BYTE = 1,
	SHORT = 3,
	LONG = 4,
};
enum TiffTags
{
	TIFF_TAG_ImageWidth = 256,
	TIFF_TAG_ImageLength = 257,
	TIFF_TAG_BitsPerSample = 258,
	TIFF_TAG_Compression = 259,
	TIFF_TAG_PhotometricInterpretation = 262,
	TIFF_TAG_StripOffsets = 273,
	TIFF_TAG_SamplesPerPixel = 277,
	TIFF_TAG_RowsPerStrip = 278,
	TIFF_TAG_StripByteCounts = 279,
	TIFF_TAG_Predictor = 317,
};
typedef struct
{
	bool is_LE;
	uint16_t format_identifier;
	uint32_t IFD_offset;
} TiffHeader;

typedef struct
{
	uint16_t tag;
	uint16_t field_type;
	uint32_t values_number;
	uint32_t values_or_offset;
} TiffIFDEntry;

typedef struct
{
	bool has_width_256;
	bool has_length_257;
	bool has_bits_per_sample_258;
	bool has_compression_259;
	bool has_photometric_262;
	bool has_strip_offsets_273;
	bool has_samples_per_pixel_277;
	bool has_rows_per_strip_278;
	bool has_strip_byte_counts_279;
} TiffVisitedTags;

typedef struct
{
	uint32_t width_256;
	uint32_t length_257;
	uint32_t *bits_per_sample_258;
	uint32_t bits_per_sample_count_258;
	uint16_t compression_259;
	uint16_t photometric_262;
	uint16_t samples_per_pixel_277;
	uint32_t rows_per_strip_278;
	uint32_t *strip_offsets_273;
	uint32_t strip_offsets_count_273;
	uint32_t *strip_byte_counts_279;
	uint32_t strip_byte_counts_count_279;
	TiffVisitedTags visited_tags;
	uint16_t predictor_317;
} TiffImageData;

void tiff_header_init(TiffHeader *header);
void tiff_IFDEntry_init(TiffIFDEntry *entry);
void tiff_TiffImageData_init(TiffImageData *tiff_data);
void tiff_TiffImageData_free(TiffImageData *tiff_data);
