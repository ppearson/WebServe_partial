/*
 WebServe
 Copyright 2018-2022 Peter Pearson.
 Originally taken from:
 Imagine
 Copyright 2011-2014 Peter Pearson.

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

#include "image_reader_png.h"

#include <png.h>

#include "image/image3f.h"

#include "data_conversion.h"

#include "image/colour_space.h"

struct PNGInfra
{
	PNGInfra() : pFile(nullptr),
		bitDepth(0),
		channels(0)
	{
	}

	FILE*			pFile;
	png_structp		pPNG;
	png_infop		pInfo;
	png_uint_32		width;
	png_uint_32		height;
	png_bytepp		pRows;
	
	int				bitDepth;

	unsigned int	channels;
};

ImageReaderPNG::ImageReaderPNG() : ImageReader()
{
}

ImageReaderPNG::ImageType ImageReaderPNG::readData(const std::string& filePath, PNGInfra& infra, bool wantAlpha) const
{
	if (filePath.empty())
		return eInvalid;

	infra.pFile = fopen(filePath.c_str(), "rb");
	if (!infra.pFile)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		return eInvalid;
	}

	unsigned char sig[8];

	// check the signature
	if (fread(sig, 8, 1, infra.pFile) != 1)
	{
		fprintf(stderr, "Cannot read header bytes from PNG file: %s.\n", filePath.c_str());
		fclose(infra.pFile);
		return eInvalid;
	}
	if (!png_check_sig(sig, 8))
	{
		fprintf(stderr, "Cannot open PNG file: %s - not a valid PNG file.\n", filePath.c_str());
		fclose(infra.pFile);
		return eInvalid;
	}

	infra.pPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!infra.pPNG)
	{
		fclose(infra.pFile);
		return eInvalid;
	}

	infra.pInfo = png_create_info_struct(infra.pPNG);
	if (!infra.pInfo)
	{
		png_destroy_read_struct(&infra.pPNG, nullptr, nullptr);
		fclose(infra.pFile);
		return eInvalid;
	}

	if (setjmp(png_jmpbuf(infra.pPNG)))
	{
		png_destroy_read_struct(&infra.pPNG, &infra.pInfo, nullptr);
		fclose(infra.pFile);
		return eInvalid;
	}

	png_init_io(infra.pPNG, infra.pFile);
	png_set_sig_bytes(infra.pPNG, 8);
	png_read_info(infra.pPNG, infra.pInfo);

	int colorType;
	int interlaceType, compressionType;

	png_get_IHDR(infra.pPNG, infra.pInfo, &infra.width, &infra.height, &infra.bitDepth, &colorType, &interlaceType, &compressionType, nullptr);

	if (colorType == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(infra.pPNG);

	if (png_get_valid(infra.pPNG, infra.pInfo, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(infra.pPNG);
	}

	ImageType type = eInvalid;

	// record what the image actually is (ish)
	if (colorType == PNG_COLOR_TYPE_RGB)
	{
		infra.channels = 3;
	}
	else if (colorType == PNG_COLOR_TYPE_RGBA)
	{
		infra.channels = 4;
	}
	else if (colorType == PNG_COLOR_TYPE_GRAY)
	{
		infra.channels = 1;
	}

	if (!wantAlpha)
	{
		// add black Alpha
		if ((colorType & PNG_COLOR_MASK_ALPHA) == 0)
			png_set_add_alpha(infra.pPNG, 0xFF, PNG_FILLER_AFTER);

		if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(infra.pPNG);
		else if (colorType != PNG_COLOR_TYPE_RGB && colorType != PNG_COLOR_TYPE_RGB_ALPHA)
			return eInvalid;

		type = eRGBA;
	}
	else
	{
		if (colorType == PNG_COLOR_TYPE_RGB_ALPHA)
			type = eRGBA;
		else if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
			type = eA;
		else
			return eInvalid;
	}

	png_read_update_info(infra.pPNG, infra.pInfo);

	infra.pRows = new png_bytep[infra.height * sizeof(png_bytep)];

	png_set_rows(infra.pPNG, infra.pInfo, infra.pRows);

	for (unsigned int i = 0; i < infra.height; i++)
	{
		infra.pRows[i] = new png_byte[png_get_rowbytes(infra.pPNG, infra.pInfo)];
	}

	png_read_image(infra.pPNG, infra.pRows);
	png_read_end(infra.pPNG, infra.pInfo);

	return type;
}

bool ImageReaderPNG::getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const
{
	PNGInfra infra;
	if (readData(filePath, infra, false) != eRGBA)
		return false;

	imageDetails.width = infra.width;
	imageDetails.height = infra.height;

	imageDetails.pixelBitDepth = infra.bitDepth;
	imageDetails.channels = infra.channels;

	return true;
}

bool ImageReaderPNG::extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const
{
	return false;
}

Image3f* ImageReaderPNG::readColour3fImage(const std::string& filePath) const
{
	PNGInfra infra;
	if (readData(filePath, infra, false) != eRGBA)
		return nullptr;
	
	Image3f* pImage3f = new Image3f(infra.width, infra.height);

	// convert to linear float

	if (infra.bitDepth == 16)
	{
		const float invShortConvert = 1.0f / 65535.0f;

		for (unsigned int i = 0; i < infra.height; i++)
		{
			png_byte* pLineData = infra.pRows[i];
			uint16_t* typedData = (uint16_t*)pLineData;

			// need to flip the height round...
			unsigned int y = infra.height - i - 1;

			Colour3f* pImageRow = pImage3f->getRowPtr(y);

			for (unsigned int x = 0; x < infra.width; x++)
			{
				uint16_t red = *typedData++;
				uint16_t green = *typedData++;
				uint16_t blue = *typedData++;
				typedData++;

				red = reverseUInt16Bytes(red);
				green = reverseUInt16Bytes(green);
				blue = reverseUInt16Bytes(blue);

				pImageRow->r = (float)red * invShortConvert;
				pImageRow->g = (float)green * invShortConvert;
				pImageRow->b = (float)blue * invShortConvert;

				ColourSpace::convertSRGBToLinearAccurate(*pImageRow);

				pImageRow++;
			}
		}
	}
	else
	{
		for (unsigned int i = 0; i < infra.height; i++)
		{
			png_byte* pLineData = infra.pRows[i];

			// need to flip the height round...
			unsigned int y = infra.height - i - 1;

			Colour3f* pImageRow = pImage3f->getRowPtr(y);

			for (unsigned int x = 0; x < infra.width; x++)
			{
				unsigned char red = *pLineData++;
				unsigned char green = *pLineData++;
				unsigned char blue = *pLineData++;
				pLineData++;

				pImageRow->r = ColourSpace::convertSRGBToLinearLUT(red);
				pImageRow->g = ColourSpace::convertSRGBToLinearLUT(green);
				pImageRow->b = ColourSpace::convertSRGBToLinearLUT(blue);

				pImageRow++;
			}
		}
	}
		
	png_destroy_read_struct(&infra.pPNG, &infra.pInfo, (png_infopp)nullptr);

	for (unsigned int y = 0; y < infra.height; y++)
	{
		delete [] infra.pRows[y];
	}
	delete [] infra.pRows;

	fclose(infra.pFile);

	return pImage3f;
}


namespace
{
	ImageReader* createImageReaderPNG()
	{
		return new ImageReaderPNG();
	}

	const bool registered = FileIORegistry::instance().registerImageReader("png", createImageReaderPNG);
}
