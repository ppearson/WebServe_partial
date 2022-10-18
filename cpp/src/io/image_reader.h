/*
 WebServe
 Copyright 2018-2022 Peter Pearson.
 Originally taken from:
 Imagine
 Copyright 2011-2015 Peter Pearson.

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

#ifndef IMAGE_READER_H
#define IMAGE_READER_H

#include <string>

#include "io/file_io_registry.h"

#include "image/image3f.h"

class ImageReader
{
public:
	ImageReader()
	{

	}

	virtual ~ImageReader()
	{

	}

	class RawEXIFMetaData
	{
	public:		
		class RawEXIFMetaDataTempPayload
		{
		public:
			RawEXIFMetaDataTempPayload()
			{
				
			}
			
			virtual ~RawEXIFMetaDataTempPayload()
			{
				
			}
		};
		
		RawEXIFMetaData() : pTempPayload(nullptr), pData(nullptr), length(0)
		{
		}

		~RawEXIFMetaData()
		{
			if (pTempPayload)
			{
				delete pTempPayload;
				pTempPayload = nullptr;
			}
		}

		RawEXIFMetaDataTempPayload*	pTempPayload;
		
		const unsigned char*	pData;
		unsigned int			length;
	};

	struct ImageDetails
	{
		ImageDetails() : width(0), height(0), pixelBitDepth(0), channels(0),
			colourSpace(ColourSpace::eUnknown)
		{
		}

		enum class ColourSpace {
			eUnknown,
			eSRGB,
			eAdobeRGB
		};

		unsigned int	width;
		unsigned int	height;

		unsigned int	pixelBitDepth;

		unsigned int	channels;

		ColourSpace		colourSpace;

		RawEXIFMetaData	exifMetadata;
	};

	virtual bool getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const = 0;

	virtual bool extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const = 0;

	// these are designed for use with loading entire planar images

	// reads in RGB colour image as floats into linear format
	virtual Image3f* readColour3fImage(const std::string& filePath) const = 0;
};

#endif // IMAGE_READER_H
