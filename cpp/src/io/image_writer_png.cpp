/*
 WebServe
 Copyright 2018-2022 Peter Pearson.
 Originally taken from:
 Imagine
 Copyright 2011-2012 Peter Pearson.

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

#include "image_writer_png.h"

#include <cstdio>

#include <png.h>

#include "maths.h"
#include "data_conversion.h"

#include "image/colour_space.h"
#include "image/image3f.h"

ImageWriterPNG::ImageWriterPNG() : ImageWriter()
{
}

bool ImageWriterPNG::writeImage(const std::string& filePath, const Image3f& image, const WriteParams& writeParams)
{
	unsigned int width = image.getWidth();
	unsigned int height = image.getHeight();
	
	png_structp pPNG;
	png_infop pInfo;

	bool save16Bit = false;
	if (writeParams.bitDepth == eBitDepth_16)
	{
		save16Bit = true;
	}
	else if (writeParams.bitDepth == eBitDepth_10 || writeParams.bitDepth == eBitDepth_12 || writeParams.bitDepth == eBitDepth_14)
	{
		save16Bit = true;
		fprintf(stderr, "Warning: An unsupport bitdepth was request for writing to PNG: %u bits, but as it is > 8, WebServe process will automatically write a 16-bit PNG.\n",
				writeParams.getRawBitDepth());
	}

	png_byte colourType = PNG_COLOR_TYPE_RGB;
	png_byte bitDepth = save16Bit ? 16 : 8;

	pPNG = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!pPNG)
		return false;

	pInfo = png_create_info_struct(pPNG);
	if (!pInfo)
	{
		png_destroy_write_struct(&pPNG, nullptr);
		return false;
	}

	/// pre-process the file

	FILE* pFile = fopen(filePath.c_str(), "wb");
	if (!pFile)
	{
		png_destroy_write_struct(&pPNG, &pInfo);
		fprintf(stderr, "Error writing PNG file: %s\n", filePath.c_str());
		return false;
	}

	png_byte** pRows = new png_byte*[height * sizeof(png_byte*)];
	if (!pRows)
	{
		png_destroy_write_struct(&pPNG, &pInfo);
		fclose(pFile);
		return false;
	}

	if (setjmp(png_jmpbuf(pPNG)))
	{
		delete [] pRows;
		png_destroy_write_struct(&pPNG, &pInfo);
		fclose(pFile);
		return false;
	}

	png_init_io(pPNG, pFile);

	png_set_IHDR(pPNG, pInfo, width, height, bitDepth, colourType, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	// TODO: again, making an assumption here... might want to support other things, with full ICC profile info in the future...
	png_set_sRGB(pPNG, pInfo, PNG_sRGB_INTENT_PERCEPTUAL);

	png_write_info(pPNG, pInfo);
	
	size_t pixelBytes = save16Bit ? 2 : 1;
	size_t byteCount = width * pixelBytes;
	
	for (unsigned int y = 0; y < height; y++)
	{
		uint8_t* row = new uint8_t[byteCount * 3];
		pRows[y] = row;
		
		// need to flip the height round...
		unsigned int actualRow = height - y - 1;
		const Colour3f* pRow = image.getRowPtr(actualRow);
		
		if (save16Bit)
		{
			uint16_t* typedRow = (uint16_t*)row;
			for (unsigned int x = 0; x < width; x++)
			{
				// TODO: again, making an assumption here... might want to support other things, with full ICC profile info in the future...
				float r = ColourSpace::convertLinearToSRGBAccurate(pRow->r);
				float g = ColourSpace::convertLinearToSRGBAccurate(pRow->g);
				float b = ColourSpace::convertLinearToSRGBAccurate(pRow->b);

				uint16_t red = (uint16_t)(MathsHelpers::clamp(r) * 65535.0f);
				uint16_t green = (uint16_t)(MathsHelpers::clamp(g) * 65535.0f);
				uint16_t blue = (uint16_t)(MathsHelpers::clamp(b) * 65535.0f);

				// TODO: this reversing should only be done on marchs which need it...
				*typedRow++ = DataConversionHelpers::reverseUInt16Bytes(red);
				*typedRow++ = DataConversionHelpers::reverseUInt16Bytes(green);
				*typedRow++ = DataConversionHelpers::reverseUInt16Bytes(blue);

				pRow++;
			}
		}
		else
		{
			for (unsigned int x = 0; x < width; x++)
			{
				// TODO: again, making an assumption here... might want to support other things, with full ICC profile info in the future...
				float r = ColourSpace::convertLinearToSRGBAccurate(pRow->r);
				float g = ColourSpace::convertLinearToSRGBAccurate(pRow->g);
				float b = ColourSpace::convertLinearToSRGBAccurate(pRow->b);

				unsigned char red = (unsigned char)(MathsHelpers::clamp(r) * 255.0f);
				unsigned char green = (unsigned char)(MathsHelpers::clamp(g) * 255.0f);
				unsigned char blue = (unsigned char)(MathsHelpers::clamp(b) * 255.0f);

				*row++ = red;
				*row++ = green;
				*row++ = blue;

				pRow++;
			}
		}
	}
	
	png_write_image(pPNG, pRows);

	for (unsigned int y = 0; y < height; y++)
	{
		delete [] pRows[y];
	}
	delete [] pRows;

	///
	
	png_write_end(pPNG, pInfo); // info ptr isn't really needed, but...

	png_destroy_write_struct(&pPNG, &pInfo);

	fclose(pFile);	
	
	return true;	
}

bool ImageWriterPNG::writeRawImageCopy(const std::string& originalFilePath, const std::string& newFilePath, const WriteRawParams& params) const
{
	FILE* pSrcFile = fopen(originalFilePath.c_str(), "rb");
	if (!pSrcFile)
	{
		fprintf(stderr, "Can't open source file: %s\n", originalFilePath.c_str());
		return false;
	}

	
	
	return false;
}


namespace
{
	ImageWriter* createImageWriterPNG()
	{
		return new ImageWriterPNG();
	}

	const bool registered = FileIORegistry::instance().registerImageWriter("png", createImageWriterPNG);
}
