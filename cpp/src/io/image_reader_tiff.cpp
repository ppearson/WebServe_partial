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

#include "image_reader_tiff.h"

#include <cstring> // for memcmp

#include <tiffio.h>

#include "image/image3f.h"

#include "image/colour_space.h"

ImageReaderTIFF::ImageReaderTIFF()
{

}

bool ImageReaderTIFF::readInfo(TIFF* pTiff, TiffInfo& tiffInfo)
{
	TIFFGetField(pTiff, TIFFTAG_IMAGELENGTH, &tiffInfo.imageHeight);
	TIFFGetField(pTiff, TIFFTAG_IMAGEWIDTH, &tiffInfo.imageWidth);
	TIFFGetField(pTiff, TIFFTAG_IMAGEDEPTH, &tiffInfo.imageDepth);

	if (tiffInfo.imageHeight == 0 || tiffInfo.imageWidth == 0)
		return false;

	TIFFGetField(pTiff, TIFFTAG_XPOSITION, &tiffInfo.xPos);
	TIFFGetField(pTiff, TIFFTAG_YPOSITION, &tiffInfo.yPos);

	TIFFGetField(pTiff, TIFFTAG_BITSPERSAMPLE, &tiffInfo.bitDepth);
	TIFFGetField(pTiff, TIFFTAG_SAMPLESPERPIXEL, &tiffInfo.channelCount);

	TIFFGetField(pTiff, TIFFTAG_ROWSPERSTRIP, &tiffInfo.rowsPerStrip);
	TIFFGetFieldDefaulted(pTiff, TIFFTAG_SAMPLEFORMAT, &tiffInfo.sampleFormat);

	uint16_t planarConfig = 0;
	TIFFGetFieldDefaulted(pTiff, TIFFTAG_PLANARCONFIG, &planarConfig);
	if (planarConfig == PLANARCONFIG_SEPARATE && tiffInfo.channelCount > 1)
	{
		tiffInfo.separatePlanes = true;
	}

	TIFFGetField(pTiff, TIFFTAG_ORIENTATION, &tiffInfo.orientation);

	TIFFGetField(pTiff, TIFFTAG_COMPRESSION, &tiffInfo.compression);

	// see if we can work out what colourspace/profile it is, as this is pretty important
	// for export from various software, otherwise we can't round-trip images correctly...

	uint16_t colorspace = 0;
	if (TIFFGetField(pTiff, EXIFTAG_COLORSPACE, &colorspace))
	{
		if (colorspace != 0xffff)
		{
			// it's got to be sRGB apparently...
			tiffInfo.colourSpace = ImageDetails::ColourSpace::eSRGB;
		}
		else
		{
			// it's likely AdobeRGB..
			// TODO: check ICC profile tag?
			tiffInfo.colourSpace = ImageDetails::ColourSpace::eAdobeRGB;
		}
	}

	uint32_t xmlLength = 0;
	const void* pXMLData = NULL;
	if (TIFFGetField(pTiff, TIFFTAG_XMLPACKET, &xmlLength, &pXMLData))
	{
		if (xmlLength > 0 && pXMLData)
		{
			std::string xmlContent((const char*)pXMLData, xmlLength);

			// TODO: something much better than this...
			// works for the moment though..
			if (xmlContent.find("CameraProfile=\"Adobe Standard\"") != std::string::npos)
			{
				// TODO: only update it if set to unknown already?
				tiffInfo.colourSpace = ImageDetails::ColourSpace::eAdobeRGB;
			}

//			fprintf(stderr, "%s\n", xmlContent.c_str());
		}
	}

	if (TIFFIsTiled(pTiff))
	{
		tiffInfo.isTiled = true;
		TIFFGetField(pTiff, TIFFTAG_TILEWIDTH, &tiffInfo.tileWidth);
		TIFFGetField(pTiff, TIFFTAG_TILELENGTH, &tiffInfo.tileHeight);
		TIFFGetField(pTiff, TIFFTAG_TILEDEPTH, &tiffInfo.tileDepth);

		// get other stuff
//		if (TIFFGetField(pTiff, TIFFTAG_PIXAR_IMAGEFULLLENGTH, &tiffInfo.imageHeight) == 1)
		{

		}

//		if (TIFFGetField(pTiff, TIFFTAG_PIXAR_IMAGEFULLWIDTH, &tiffInfo.imageWidth) == 1)
		{

		}
	}

	return true;
}

bool ImageReaderTIFF::getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const
{
	TIFF* pTiff = TIFFOpen(filePath.c_str(), "r");
	if (!pTiff)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		return false;
	}

	TiffInfo tiffInfo;
	if (!readInfo(pTiff, tiffInfo))
	{
		fprintf(stderr, "Invalid tiff file: %s\n", filePath.c_str());
		TIFFClose(pTiff);
		return false;
	}

	imageDetails.width = tiffInfo.imageWidth;
	imageDetails.height = tiffInfo.imageHeight;

	imageDetails.pixelBitDepth = tiffInfo.bitDepth;
	imageDetails.channels = tiffInfo.channelCount;

	imageDetails.colourSpace = tiffInfo.colourSpace;

	TIFFClose(pTiff);

	return true;
}

bool ImageReaderTIFF::extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const
{
	TIFF* pTiff = TIFFOpen(filePath.c_str(), "r");
	if (!pTiff)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		return false;
	}

	TiffInfo tiffInfo;
	if (!readInfo(pTiff, tiffInfo))
	{
		fprintf(stderr, "Invalid tiff file: %s\n", filePath.c_str());
		TIFFClose(pTiff);
		return false;
	}


	TIFFClose(pTiff);


	return false;
}

Image3f* ImageReaderTIFF::readColour3fImage(const std::string& filePath) const
{
	TIFF* pTiff = TIFFOpen(filePath.c_str(), "r");
	if (!pTiff)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		return nullptr;
	}

	TiffInfo tiffInfo;
	if (!readInfo(pTiff, tiffInfo))
	{
		fprintf(stderr, "Invalid tiff file: %s\n", filePath.c_str());
		TIFFClose(pTiff);
		return nullptr;
	}

	if (!tiffInfo.isTiled)
	{
		return readScanlineColourImage(filePath, pTiff, tiffInfo);
	}
	else
	{
		return readTiledColourImage(filePath, pTiff, tiffInfo);
	}

	TIFFClose(pTiff);

	return nullptr;
}

Image3f* ImageReaderTIFF::readScanlineColourImage(const std::string& filePath, TIFF* pTiff, TiffInfo& tiffInfo)
{
	Image3f* pImage3f = new Image3f(tiffInfo.imageWidth, tiffInfo.imageHeight);

	if (tiffInfo.bitDepth == 8)
	{
		// allocate memory for image - do it the easy way for the moment - we can convert to scanline approach later...

		uint32_t totalImageSize = tiffInfo.imageHeight * tiffInfo.imageWidth;
		uint32_t* pRawBuffer = (uint32_t*)_TIFFmalloc(totalImageSize * sizeof(uint32_t)); // RGBA byte

		if (!pRawBuffer)
		{
			fprintf(stderr, "Couldn't allocate memory to read file: %s\n", filePath.c_str());
			delete pImage3f;
			return nullptr;
		}

		if (TIFFReadRGBAImage(pTiff, tiffInfo.imageWidth, tiffInfo.imageHeight, pRawBuffer, 0) == 0)
		{
			fprintf(stderr, "Couldn't read image: %s\n", filePath.c_str());

			_TIFFfree(pRawBuffer);

			delete pImage3f;
			return nullptr;
		}

		unsigned int index = 0;

		for (unsigned int i = 0; i < tiffInfo.imageHeight; i++)
		{
			// we don't seem to need to reverse this for 8-bit...
			unsigned int y = i;

			Colour3f* pImageRow = pImage3f->getRowPtr(y);

			for (unsigned int x = 0; x < tiffInfo.imageWidth; x++)
			{
				unsigned char red = TIFFGetR(pRawBuffer[index]);
				unsigned char green = TIFFGetG(pRawBuffer[index]);
				unsigned char blue = TIFFGetB(pRawBuffer[index]);

				pImageRow->r = ColourSpace::convertSRGBToLinearLUT(red);
				pImageRow->g = ColourSpace::convertSRGBToLinearLUT(green);
				pImageRow->b = ColourSpace::convertSRGBToLinearLUT(blue);

				pImageRow++;

				index ++;
			}
		}

		_TIFFfree(pRawBuffer);
	}
	else
	{
		// if we're not 8-bit, it's highly likely the image is compressed, so
		// we need to read images as strips, as TIFFReadScanline() cannot read
		// compressed images.

		unsigned int stripSize = TIFFStripSize(pTiff);

		unsigned int numStrips = TIFFNumberOfStrips(pTiff);

		tdata_t pRawBuffer = _TIFFmalloc(stripSize);

		if (!pRawBuffer)
		{
			fprintf(stderr, "Couldn't allocate memory to read file: %s\n", filePath.c_str());

			if (pImage3f)
			{
				delete pImage3f;
				pImage3f = nullptr;
			}

			TIFFClose(pTiff);
			return nullptr;
		}

		unsigned int targetY = 0;

		const float invShortConvert = 1.0f / 65535.0f;

		// Note: the colourspace conversion happens at the end before returning the image item...

		for (unsigned int strip = 0; strip < numStrips; strip++)
		{
			TIFFReadEncodedStrip(pTiff, strip, pRawBuffer, (tsize_t)-1);

			for (unsigned int tY = 0; tY < tiffInfo.rowsPerStrip; tY++)
			{
				if (tiffInfo.bitDepth == 16)
				{
					// TODO: there's got to be a better way to handle this... Return value of TIFFReadEncodedStrip()?
					if (targetY >= tiffInfo.imageHeight)
						break;

					const uint16_t* pUShortLine = (uint16_t*)pRawBuffer + (tY * tiffInfo.imageWidth * tiffInfo.channelCount);

					// reverse Y
					unsigned int actualY = tiffInfo.imageHeight - targetY - 1;
					Colour3f* pImageRow = pImage3f->getRowPtr(actualY);

					for (unsigned int x = 0; x < tiffInfo.imageWidth; x++)
					{
						uint16_t red = *pUShortLine++;
						uint16_t green = *pUShortLine++;
						uint16_t blue = *pUShortLine++;

						pImageRow->r = (float)red * invShortConvert;
						pImageRow->g = (float)green * invShortConvert;
						pImageRow->b = (float)blue * invShortConvert;

						// Note: colourspace conversion is handled below...

						pImageRow++;
					}
				}
				else
				{
					if (targetY >= tiffInfo.imageHeight)
						break;

					const float* pFloatLine = (float*)pRawBuffer + (tY * tiffInfo.imageWidth * tiffInfo.channelCount);

					// reverse Y
					unsigned int actualY = tiffInfo.imageHeight - targetY - 1;
					Colour3f* pImageRow = pImage3f->getRowPtr(actualY);

					for (unsigned int x = 0; x < tiffInfo.imageWidth; x++)
					{
						pImageRow->r = *pFloatLine++;
						pImageRow->g = *pFloatLine++;
						pImageRow->b = *pFloatLine++;

						pImageRow++;
					}
				}

				targetY += 1;
			}
		}

		// Now do colourspace conversion in one convenient place if we're not 32-bit float...
		// TODO: and for 8-bit this way, instead of using the LUT above?
		if (tiffInfo.bitDepth == 16)
		{
			unsigned int numPixels = pImage3f->getWidth() * pImage3f->getHeight();
			Colour3f* pPixel = pImage3f->getRowPtr(0);

			if (tiffInfo.colourSpace == ImageDetails::ColourSpace::eAdobeRGB)
			{
				// we know (we think) it's Adobe RGB, so convert from that.

				for (unsigned int i = 0; i < numPixels; i++)
				{
					ColourSpace::convertAdobeRGBToLinearAccurate(*pPixel);
					pPixel++;
				}

			}
			else
			{
				// otherwise, we either don't know what it is or it's sRGB,
				// either way just assume it's sRGB...

				for (unsigned int i = 0; i < numPixels; i++)
				{
					ColourSpace::convertSRGBToLinearAccurate(*pPixel);
					pPixel++;
				}
			}
		}

		_TIFFfree(pRawBuffer);
	}

	return pImage3f;
}

Image3f* ImageReaderTIFF::readTiledColourImage(const std::string& filePath, TIFF* pTiff, TiffInfo& tiffInfo)
{
	Image3f* pImage3f = new Image3f(tiffInfo.imageWidth, tiffInfo.imageHeight);

	// allocate memory to store a single tile
	unsigned int tileLineSize = (tiffInfo.bitDepth / 8) * tiffInfo.tileWidth * tiffInfo.channelCount;
	unsigned int tileByteSize = tileLineSize * tiffInfo.tileHeight;

	unsigned char* pTileBuffer = new unsigned char[tileByteSize];

	// work out how many tiles we've got in each direction
	unsigned int tileCountX = tiffInfo.imageWidth / tiffInfo.tileWidth;
	unsigned int tileCountY = tiffInfo.imageHeight / tiffInfo.tileHeight;

	// see if there are any remainders, meaning non-complete tiles...
	bool remainderX = false;
	bool remainderY = false;

	if (tiffInfo.imageWidth % tiffInfo.tileWidth > 0)
	{
		remainderX = true;
		tileCountX += 1;
	}

	if (tiffInfo.imageHeight % tiffInfo.tileHeight > 0)
	{
		remainderY = true;
		tileCountY += 1;
	}

	const float invShortConvert = 1.0f / 65535.0f;

	// we need to read each tile individually and copy it into the destination image - this is not going to be too efficient...
	// TODO: need to work out if tile order makes a difference - rows first or columns?

	for (unsigned int tileX = 0; tileX < tileCountX; tileX++)
	{
		unsigned int tilePosX = tileX * tiffInfo.tileWidth;

		unsigned int localTileWidth = tiffInfo.tileWidth;
		if (remainderX && tileX == (tileCountX - 1))
		{
			localTileWidth = tiffInfo.imageWidth - ((tileCountX - 1) * tiffInfo.tileWidth);
		}

		for (unsigned int tileY = 0; tileY < tileCountY; tileY++)
		{
			unsigned int tilePosY = tileY * tiffInfo.tileHeight;

			TIFFReadTile(pTiff, pTileBuffer, tilePosX, tilePosY, 0, 0);

			unsigned int localTileHeight = tiffInfo.tileHeight;
			if (remainderY && tileY == (tileCountY - 1))
			{
				localTileHeight = tiffInfo.imageHeight - ((tileCountY - 1) * tiffInfo.tileHeight);
			}

			unsigned int localYStartPos = tileY * tiffInfo.tileHeight;

			// now copy the data into our image in the correct position...

			if (tiffInfo.bitDepth == 32)
			{
				// cast to the type the data should be...
				const Colour3f* pSrcTileBuffer = (Colour3f*)pTileBuffer;

				unsigned int localLineSizeBytes = (tiffInfo.bitDepth / 8) * localTileWidth * tiffInfo.channelCount;

				for (unsigned int localY = 0; localY < localTileHeight; localY++)
				{
					// reverse Y
					unsigned int actualY = tiffInfo.imageHeight - (localYStartPos + localY + 1);
					Colour3f* pDst = pImage3f->getRowPtr(actualY);

					// offset to X pos
					pDst += tilePosX;

					memcpy(pDst, pSrcTileBuffer, localLineSizeBytes);

					// because we're of the correct type, we should be able to just increment
					// by the tile width to get to the next line...
					pSrcTileBuffer += tiffInfo.tileWidth;
				}
			}
			else if (tiffInfo.bitDepth == 16)
			{
				// cast to the type the data should be...
				const uint16_t* pSrcTileBuffer = (uint16_t*)pTileBuffer;

				for (unsigned int localY = 0; localY < localTileHeight; localY++)
				{
					// reverse Y
					unsigned int actualY = tiffInfo.imageHeight - (localYStartPos + localY + 1);
					Colour3f* pDst = pImage3f->getRowPtr(actualY);

					// offset to X pos
					pDst += tilePosX;

					const uint16_t* pLocalSrcTileBuffer = pSrcTileBuffer + (localY * tiffInfo.tileWidth * 3); // need to use tileWidth here

					for (unsigned int localX = 0; localX < localTileWidth; localX++)
					{
						uint16_t red = *pLocalSrcTileBuffer++;
						uint16_t green = *pLocalSrcTileBuffer++;
						uint16_t blue = *pLocalSrcTileBuffer++;

						pDst->r = (float)red * invShortConvert;
						pDst->g = (float)green * invShortConvert;
						pDst->b = (float)blue * invShortConvert;

						// convert to linear
						ColourSpace::convertSRGBToLinearAccurate(*pDst++);
					}
				}
			}
			else if (tiffInfo.bitDepth == 8)
			{
				// cast to the type the data should be...
				const uint8_t* pSrcTileBuffer = (uint8_t*)pTileBuffer;

				for (unsigned int localY = 0; localY < localTileHeight; localY++)
				{
					// reverse Y
					unsigned int actualY = tiffInfo.imageHeight - (localYStartPos + localY + 1);
					Colour3f* pDst = pImage3f->getRowPtr(actualY);

					// offset to X pos
					pDst += tilePosX;

					const uint8_t* pLocalSrcTileBuffer = pSrcTileBuffer + (localY * tiffInfo.tileWidth * 3); // need to use tileWidth here

					for (unsigned int localX = 0; localX < localTileWidth; localX++)
					{
						uint8_t red = *pLocalSrcTileBuffer++;
						uint8_t green = *pLocalSrcTileBuffer++;
						uint8_t blue = *pLocalSrcTileBuffer++;

						pDst->r = ColourSpace::convertSRGBToLinearLUT(red);
						pDst->g = ColourSpace::convertSRGBToLinearLUT(green);
						pDst->b = ColourSpace::convertSRGBToLinearLUT(blue);

						pDst++;
					}
				}
			}
		}
	}

	if (pTileBuffer)
	{
		delete [] pTileBuffer;
		pTileBuffer = nullptr;
	}

	TIFFClose(pTiff);

	return pImage3f;
}

namespace
{
	ImageReader* createImageReaderTIFF()
	{
		return new ImageReaderTIFF();
	}

	const bool registered = FileIORegistry::instance().registerImageReaderMultipleExtensions("tif;tiff;tex", createImageReaderTIFF);
}
