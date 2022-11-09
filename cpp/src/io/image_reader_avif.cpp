/*
 WebServe
 Copyright 2018-2022 Peter Pearson.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

#include "image_reader_avif.h"

#include <avif/avif.h>

#include <memory.h>

#include "image/image3f.h"

#include "image/colour_space.h"

ImageReaderAVIF::ImageReaderAVIF()
{

}

bool ImageReaderAVIF::getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const
{
	avifDecoder* decoder = avifDecoderCreate();

	avifResult result = avifDecoderSetIOFile(decoder, filePath.c_str());
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		fprintf(stderr, "Cannot open file: %s\n", filePath.c_str());
		return false;
	}

	if (!extractEXIF)
	{
		decoder->ignoreExif = true;
	}

	result = avifDecoderParse(decoder);
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		fprintf(stderr, "Failed to parse AVIF image: '%s' : %s\n", filePath.c_str(), avifResultToString(result));
		return false;
	}

	imageDetails.width = decoder->image->width;
	imageDetails.height = decoder->image->height;

	imageDetails.channels = 3;
	if (decoder->image->alphaPlane)
	{
		imageDetails.channels = 4;
	}
	imageDetails.pixelBitDepth = decoder->image->depth;

	if (extractEXIF)
	{
		if (decoder->image->exif.data && decoder->image->exif.size > 0)
		{
			imageDetails.exifMetadata.pData = decoder->image->exif.data;
			imageDetails.exifMetadata.length = decoder->image->exif.size;
		}
	}

	avifDecoderDestroy(decoder);

	return true;
}

bool ImageReaderAVIF::extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const
{
	avifDecoder* decoder = avifDecoderCreate();

	avifResult result = avifDecoderSetIOFile(decoder, filePath.c_str());
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		fprintf(stderr, "Cannot open file: %s\n", filePath.c_str());
		return false;
	}

	result = avifDecoderParse(decoder);
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		fprintf(stderr, "Failed to parse AVIF image: '%s' : %s\n", filePath.c_str(), avifResultToString(result));
		return false;
	}

	if (decoder->image->exif.data && decoder->image->exif.size > 0)
	{
		exifData.pData = decoder->image->exif.data;
		exifData.length = decoder->image->exif.size;

		avifDecoderDestroy(decoder);

		return true;
	}

	avifDecoderDestroy(decoder);

	// likely no EXIF metadata...
	return false;
}

Image3f* ImageReaderAVIF::readColour3fImage(const std::string& filePath) const
{
	avifDecoder* decoder = avifDecoderCreate();

	avifResult result = avifDecoderSetIOFile(decoder, filePath.c_str());
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		fprintf(stderr, "Cannot open file: %s\n", filePath.c_str());
		return nullptr;
	}

	result = avifDecoderParse(decoder);
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		fprintf(stderr, "Failed to parse AVIF image: '%s' : %s\n", filePath.c_str(), avifResultToString(result));
		return nullptr;
	}

	unsigned int depth = decoder->image->depth;

	unsigned int width = decoder->image->width;
	unsigned int height = decoder->image->height;

	Image3f* pImage3f = new Image3f(width, height);

	// just the first one for the moment...
	if (avifDecoderNextImage(decoder) != AVIF_RESULT_OK)
	{
		fprintf(stderr, "Failed to decode AVIF image for file: %s...\n", filePath.c_str());
		avifDecoderDestroy(decoder);

		delete pImage3f;

		return nullptr;
	}

	avifRGBImage rgb;
	memset(&rgb, 0, sizeof(rgb));

	avifRGBImageSetDefaults(&rgb, decoder->image);

	// we don't want any alpha
	rgb.ignoreAlpha = true;
	rgb.format = AVIF_RGB_FORMAT_RGB;

	avifRGBImageAllocatePixels(&rgb);

	if (avifImageYUVToRGB(decoder->image, &rgb) != AVIF_RESULT_OK)
	{
	   fprintf(stderr, "Error converting AVIF pixels from YUV for file: %s\n", filePath.c_str());
	   avifRGBImageFreePixels(&rgb);
	   avifDecoderDestroy(decoder);

	   delete pImage3f;

	   return nullptr;
	}

	if (depth > 8)
	{
		uint16_t* startPixel = (uint16_t*)rgb.pixels;

		float invConvert = 1.0f / (float)(1 << depth);

		for (unsigned int i = 0; i < height; i++)
		{
			// need to flip the height round...
			unsigned int y = height - i - 1;

			Colour3f* pImageRow = pImage3f->getRowPtr(y);

			const uint16_t* pSrcPixel = startPixel + ((width * i) * 3);

			for (unsigned int x = 0; x < width; x++)
			{
				uint16_t red = *pSrcPixel++;
				uint16_t green = *pSrcPixel++;
				uint16_t blue = *pSrcPixel++;

				pImageRow->r = (float)red * invConvert;
				pImageRow->g = (float)green * invConvert;
				pImageRow->b = (float)blue * invConvert;

				ColourSpace::convertSRGBToLinearAccurate(*pImageRow);

				pImageRow++;
			}
		}
	}
	else
	{
		// it's 8-bit, so we can ingest more directly...

		for (unsigned int i = 0; i < height; i++)
		{
			// need to flip the height round...
			unsigned int y = height - i - 1;

			Colour3f* pImageRow = pImage3f->getRowPtr(y);

			const uint8_t* pSrcPixel = rgb.pixels + ((width * i) * 3);

			for (unsigned int x = 0; x < width; x++)
			{
				uint8_t red = *pSrcPixel++;
				uint8_t green = *pSrcPixel++;
				uint8_t blue = *pSrcPixel++;

				pImageRow->r = ColourSpace::convertSRGBToLinearLUT(red);
				pImageRow->g = ColourSpace::convertSRGBToLinearLUT(green);
				pImageRow->b = ColourSpace::convertSRGBToLinearLUT(blue);

				pImageRow++;
			}
		}
	}

	avifRGBImageFreePixels(&rgb);
	avifDecoderDestroy(decoder);

	return pImage3f;
}

namespace
{
	ImageReader* createImageReaderAVIF()
	{
		return new ImageReaderAVIF();
	}

	const bool registered = FileIORegistry::instance().registerImageReaderMultipleExtensions("avif", createImageReaderAVIF);
}
