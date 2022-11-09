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

#ifndef IMAGE_READER_EXR_H
#define IMAGE_READER_EXR_H

#include "io/image_reader.h"

class ImageReaderEXR : public ImageReader
{
public:
	ImageReaderEXR();

	struct EXRInfo
	{
		EXRInfo() : width(0), height(0), tiled(false), increasingY(false), channelFlags(0)
		{

		}

		enum EXRDataType
		{
			eFloat,
			eHalf,
			eUInt
		};

		enum ChannelFlags
		{
			eRed		= 1 << 0,
			eGreen		= 1 << 1,
			eBlue		= 1 << 2,
			eAlpha		= 1 << 3
		};

		struct EXRChannel
		{
			EXRChannel()
			{
			}

			EXRChannel(const std::string& channelName) : name(channelName), index(-1)
			{
			}

			std::string			name;
			EXRDataType			dataType;
			unsigned int		index;
		};

		unsigned int		width;
		unsigned int		height;
		bool				tiled;
		bool				increasingY;

		unsigned int		channelFlags;

		bool hasChannelFlags(unsigned int flags) const
		{
			return ((channelFlags & flags) == flags);
		}

		bool hasChannel(const std::string& name) const
		{
			std::vector<EXRChannel>::const_iterator it = aChannels.begin();
			for (; it != aChannels.end(); ++it)
			{
				const std::string& channelName = (*it).name;

				if (channelName == name)
					return true;
			}

			return false;
		}

		std::vector<EXRChannel>	aChannels;
	};

	bool readMetadata(const std::string& filePath, EXRInfo& info) const;

	virtual bool getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const override;

	virtual bool extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const override;

	virtual Image3f* readColour3fImage(const std::string& filePath) const override;

protected:
};

#endif // IMAGE_READER_EXR_H
