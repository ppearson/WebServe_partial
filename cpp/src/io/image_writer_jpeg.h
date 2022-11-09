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

#ifndef IMAGE_WRITER_JPEG_H
#define IMAGE_WRITER_JPEG_H

#include "io/image_writer.h"

class ImageWriterJPEG : public ImageWriter
{
public:
	ImageWriterJPEG();
	
	virtual bool writeImage(const std::string& filePath, const Image3f& image, const WriteParams& writeParams) override;
	
	virtual bool writeRawImageCopy(const std::string& originalFilePath, const std::string& newFilePath, const WriteRawParams& params) const override;
	
	
protected:
	static void copyRawMarkers(struct jpeg_decompress_struct* pDInfo, struct jpeg_compress_struct* pCInfo, const WriteRawParams& params);
};

#endif // IMAGE_WRITER_JPEG_H
