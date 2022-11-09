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

#include "image_writer_avif.h"

#include <avif/avif.h>

#include <memory.h>

#include "maths.h"
#include "image/image3f.h"

#include "image/colour_space.h"

ImageWriterAVIF::ImageWriterAVIF() : ImageWriter()
{

}

bool ImageWriterAVIF::writeImage(const std::string& filePath, const Image3f& image, const WriteParams& writeParams)
{
	unsigned int width = image.getWidth();
	unsigned int height = image.getHeight();

	avifRWData avifOutput = AVIF_DATA_EMPTY;
	avifRGBImage rgb;
	memset(&rgb, 0, sizeof(rgb));

	uint32_t rawBitDepth = writeParams.getRawBitDepth();

	avifPixelFormat pixelFormatChromaSubsampling = AVIF_PIXEL_FORMAT_YUV444;
	if (writeParams.chromaSubSamplingType == ImageWriter::eChromaSS_422)
	{
		pixelFormatChromaSubsampling = AVIF_PIXEL_FORMAT_YUV422;
	}

	avifImage* newImage = avifImageCreate((int)width, (int)height, rawBitDepth, pixelFormatChromaSubsampling);

	avifRGBImageSetDefaults(&rgb, newImage);

	rgb.format = AVIF_RGB_FORMAT_RGB;

	avifRGBImageAllocatePixels(&rgb);

	if (rawBitDepth == 8)
	{
		static constexpr float kByteMult = 255.0f;
		for (unsigned int y = 0; y < height; y++)
		{
			// need to flip the height round...
			unsigned int actualRow = height - y - 1;
			const Colour3f* pRow = image.getRowPtr(actualRow);

			uint8_t* pDstPixel = rgb.pixels + ((width * y) * 3);

			for (unsigned int x = 0; x < width; x++)
			{
				float r = ColourSpace::convertLinearToSRGBAccurate(pRow->r);
				float g = ColourSpace::convertLinearToSRGBAccurate(pRow->g);
				float b = ColourSpace::convertLinearToSRGBAccurate(pRow->b);

				uint8_t red = (uint8_t)(MathsHelpers::clamp(r) * kByteMult);
				uint8_t green = (uint8_t)(MathsHelpers::clamp(g) * kByteMult);
				uint8_t blue = (uint8_t)(MathsHelpers::clamp(b) * kByteMult);

				*pDstPixel++ = red;
				*pDstPixel++ = green;
				*pDstPixel++ = blue;

				pRow++;
			}
		}
	}
	else
	{
		// treat the backing pixels as uint16_t, however we need to convert the uint16_t values to the correct bit depth, as libavif won't do
		// that for us, so we need to scale the values appropriately for the bit depth required.

		const uint16_t kByteMult = 1 << rawBitDepth;
		const float kByteMultFloat = (float)kByteMult;
		for (unsigned int y = 0; y < height; y++)
		{
			// need to flip the height round...
			unsigned int actualRow = height - y - 1;
			const Colour3f* pRow = image.getRowPtr(actualRow);

			uint16_t* pDstPixel = ((uint16_t*)rgb.pixels) + ((width * y) * 3);

			for (unsigned int x = 0; x < width; x++)
			{
				float r = ColourSpace::convertLinearToSRGBAccurate(pRow->r);
				float g = ColourSpace::convertLinearToSRGBAccurate(pRow->g);
				float b = ColourSpace::convertLinearToSRGBAccurate(pRow->b);

				uint16_t red = (uint16_t)(MathsHelpers::clamp(r) * kByteMultFloat);
				uint16_t green = (uint16_t)(MathsHelpers::clamp(g) * kByteMultFloat);
				uint16_t blue = (uint16_t)(MathsHelpers::clamp(b) * kByteMultFloat);

				*pDstPixel++ = red;
				*pDstPixel++ = green;
				*pDstPixel++ = blue;

				pRow++;
			}
		}
	}

	avifResult convertResult = avifImageRGBToYUV(newImage, &rgb);
	if (convertResult != AVIF_RESULT_OK)
	{
		fprintf(stderr, "Failed to convert to YUV(A): %s\n", avifResultToString(convertResult));
		return false;
	}

	avifEncoder* encoder = avifEncoderCreate();

	encoder->codecChoice = AVIF_CODEC_CHOICE_AOM;
	encoder->maxThreads = 4;

	// AVIF quality is 0 == best -> 63 == worst, so scale inversely.

	encoder->minQuantizer = 5;
	encoder->maxQuantizer = 10;

	// speed = 0 -> 10

	encoder->speed = 3;

	avifResult addImageResult = avifEncoderAddImage(encoder, newImage, 1, AVIF_ADD_IMAGE_FLAG_SINGLE);
	if (addImageResult != AVIF_RESULT_OK)
	{
		fprintf(stderr, "Failed to add image to encoder: %s\n", avifResultToString(addImageResult));

		if (newImage)
		{
			avifImageDestroy(newImage);
		}
		if (encoder)
		{
			avifEncoderDestroy(encoder);
		}
		avifRWDataFree(&avifOutput);
		avifRGBImageFreePixels(&rgb);

		return false;
	}

	avifResult finishResult = avifEncoderFinish(encoder, &avifOutput);
	if (finishResult != AVIF_RESULT_OK)
	{
		fprintf(stderr, "Failed to finish encode: %s\n", avifResultToString(finishResult));

		if (newImage)
		{
			avifImageDestroy(newImage);
		}
		if (encoder)
		{
			avifEncoderDestroy(encoder);
		}
		avifRWDataFree(&avifOutput);
		avifRGBImageFreePixels(&rgb);

		return false;
	}

	FILE* file = fopen(filePath.c_str(), "wb");
	size_t bytesWritten = fwrite(avifOutput.data, 1, avifOutput.size, file);
	fclose(file);

	bool success = true;
	if (bytesWritten != avifOutput.size)
	{
		success = false;
	}

	if (newImage)
	{
		avifImageDestroy(newImage);
	}
	if (encoder)
	{
		avifEncoderDestroy(encoder);
	}
	avifRWDataFree(&avifOutput);
	avifRGBImageFreePixels(&rgb);

	return success;
}

namespace
{
	ImageWriter* createImageWriterAVIF()
	{
		return new ImageWriterAVIF();
	}

	const bool registered = FileIORegistry::instance().registerImageWriter("avif", createImageWriterAVIF);
}
