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

#ifndef IMAGE_READER_HEIF_H
#define IMAGE_READER_HEIF_H

#include "io/image_reader.h"

class ImageReaderHEIF : public ImageReader
{
public:
	ImageReaderHEIF();

	class HEIFRawEXIFMetaDataPayload : public ImageReader::RawEXIFMetaData::RawEXIFMetaDataTempPayload
	{
	public:
		HEIFRawEXIFMetaDataPayload() : RawEXIFMetaData::RawEXIFMetaDataTempPayload(),
		  pOriginalPayload(nullptr), originalSize(0)
		{

		}

		virtual ~HEIFRawEXIFMetaDataPayload();

		// because we need to offset into address, as HEIC (at least from iPhone 13) seem to have 4 bytes of
		// padding before Exif header starts, we need to keep track of original allocation...
		uint8_t*		pOriginalPayload;
		unsigned int	originalSize;
	};

	virtual bool getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const override;

	virtual bool extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const override;

	virtual Image3f* readColour3fImage(const std::string& filePath) const override;
};

#endif // IMAGE_READER_HEIF_H
