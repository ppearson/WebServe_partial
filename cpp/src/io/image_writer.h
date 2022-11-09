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

#ifndef IMAGE_WRITER_H
#define IMAGE_WRITER_H

#include <string>

#include "io/file_io_registry.h"

class Image3f;

class ImageWriter
{
public:
	ImageWriter()
	{
	}

	virtual ~ImageWriter()
	{
	}

	enum ChromaSubSamplingType
	{
		eChromaSS_411,
		eChromaSS_422,
		eChromaSS_444
	};

	enum BitDepth
	{
		eBitDepth_8,
		eBitDepth_10,
		eBitDepth_12,
		eBitDepth_14,
		eBitDepth_16,
		eBitDepth_32
	};

	struct WriteParams
	{
		WriteParams(float qual, ChromaSubSamplingType chromaSSType) :
			quality(qual),
			chromaSubSamplingType(chromaSSType)
		{

		}

		BitDepth				bitDepth				= eBitDepth_8;

		float					quality					= 0.96f;
		ChromaSubSamplingType	chromaSubSamplingType	= eChromaSS_444;


		uint32_t getRawBitDepth() const
		{
			switch (bitDepth)
			{
				case eBitDepth_8:  return 8;
				case eBitDepth_10: return 10;
				case eBitDepth_12: return 12;
				case eBitDepth_14: return 14;
				case eBitDepth_16: return 16;
				case eBitDepth_32: return 32;
			}
			return 0;
		}
	};


	virtual bool writeImage(const std::string& filePath, const Image3f& image, const WriteParams& writeParams) = 0;
	
	struct WriteRawParams
	{
		WriteRawParams()
		{
			
		}
		
		bool	writeMetadata = true;
		bool	writeEXIFMetadata = true;
		
		bool	stripXMPMetadata = true;
	};
	
	virtual bool writeRawImageCopy(const std::string& originalFilePath, const std::string& newFilePath, const WriteRawParams& params) const
	{
		return false;
	}
};

#endif // IMAGE_WRITER_H
