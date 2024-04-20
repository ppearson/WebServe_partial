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

#ifndef IMAGE_READER_TIFF_H
#define IMAGE_READER_TIFF_H

#include <stdint.h>

#include "io/image_reader.h"

struct tiff;
typedef struct tiff TIFF;

class ImageReaderTIFF : public ImageReader
{
public:
	ImageReaderTIFF();

	virtual bool getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const override;

	virtual bool extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const override;

	virtual Image3f* readColour3fImage(const std::string& filePath) const override;

protected:
	struct TiffInfo
	{
		TiffInfo() : imageHeight(0), imageWidth(0), rowsPerStrip(0), bitDepth(0), channelCount(0), orientation(0),
			sampleFormat(0), compression(0), separatePlanes(false), isTiled(false), tileWidth(0), tileHeight(0), tileDepth(0),
			colourSpace(ImageDetails::ColourSpace::eUnknown)
		{
		}

		bool sampleFormatIsIEEEFloat() const
		{
			return sampleFormat == 3; // SAMPLEFORMAT_IEEEFP
		}

		uint32_t	imageHeight;
		uint32_t	imageWidth;
		uint32_t	imageDepth;
		uint32_t	rowsPerStrip;
		uint16_t	bitDepth;
		uint16_t	channelCount;
		uint16_t	orientation;
		uint16_t	sampleFormat;
		uint32_t	compression;

		bool		separatePlanes; // each channel is stored in separate planes

		uint32_t	xPos;
		uint32_t	yPos;

		bool		isTiled;
		uint32_t	tileWidth;
		uint32_t	tileHeight;
		uint16_t	tileDepth;

		ImageDetails::ColourSpace	colourSpace;
	};

	static bool readInfo(TIFF* pTiff, TiffInfo& tiffInfo);

	static Image3f* readScanlineColourImage(const std::string& filePath, TIFF* pTiff, TiffInfo& tiffInfo);
	static Image3f* readTiledColourImage(const std::string& filePath, TIFF* pTiff, TiffInfo& tiffInfo);
};

#endif // IMAGE_READER_TIFF_H
