/*
 WebServe
 Copyright 2018-2019 Peter Pearson.
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

	// this is somewhat imperfect, as currently these params are
	// jpeg-specific, but in theory other formats might use something similar, so...

	enum ChromaSubSamplingType
	{
		eChromaSS_411,
		eChromaSS_422,
		eChromaSS_444
	};

	struct WriteParams
	{
		WriteParams(float qual, ChromaSubSamplingType chromaSSType) :
			quality(qual),
			chromaSubSamplingType(chromaSSType)
		{

		}

		float					quality					= 0.96f;
		ChromaSubSamplingType	chromaSubSamplingType	= eChromaSS_422;
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
