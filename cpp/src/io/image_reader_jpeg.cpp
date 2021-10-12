/*
 WebServe
 Copyright 2018 Peter Pearson.
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

#include "image_reader_jpeg.h"

#include <cstring> // for memcmp

#include <jpeglib.h>

#include "image/image3f.h"

#include "image/colour_space.h"


ImageReaderJPEG::ImageReaderJPEG()
{
}

ImageReaderJPEG::JPEGRawEXIFMetaDataPayload::~JPEGRawEXIFMetaDataPayload()
{
	if (pDecompressStruct)
	{
		jpeg_destroy_decompress(pDecompressStruct);
		delete pDecompressStruct;
		pDecompressStruct = nullptr;
	}
}

bool ImageReaderJPEG::getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const
{
	FILE* pFile = fopen(filePath.c_str(), "rb");
	if (!pFile)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		return false;
	}

	struct jpeg_decompress_struct cinfo;

	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, pFile);
	if (extractEXIF)
	{
		jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);
	}
	jpeg_read_header(&cinfo, TRUE);
	if (jpeg_start_decompress(&cinfo) != TRUE)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		jpeg_destroy_decompress(&cinfo);
		fclose(pFile);
		return false;
	}

//	int depth = cinfo.output_components;

	imageDetails.width = (unsigned int)cinfo.output_width;
	imageDetails.height = (unsigned int)cinfo.output_height;

	if (extractEXIF)
	{
		jpeg_saved_marker_ptr pMarker = cinfo.marker_list;

		while (pMarker != nullptr)
		{
			if (pMarker->marker == 0xe1) // 225
			{
				// we only care about the EXIF one for the moment...
				if (memcmp(pMarker->data, "Exif", 4) == 0)
				{
					imageDetails.exifMetadata.pData = pMarker->data;
					imageDetails.exifMetadata.length = pMarker->data_length;
					break;
				}
			}
			pMarker = pMarker->next;
		}
	}

	jpeg_destroy_decompress(&cinfo);
	fclose(pFile);

	return true;
}

bool ImageReaderJPEG::extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const
{
	FILE* pFile = fopen(filePath.c_str(), "rb");
	if (!pFile)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		return false;
	}

	// allocate a temp payload in the metadata struct, such that we can close the file down
	// but keep the metadata memory alive for a bit longer (the duration of the RawEXIFMetaData struct)
	ImageReaderJPEG::JPEGRawEXIFMetaDataPayload* pTypedPayload = new ImageReaderJPEG::JPEGRawEXIFMetaDataPayload();
	// we purposefully allocate this within the 
	exifData.pTempPayload = pTypedPayload;
	
	pTypedPayload->pDecompressStruct = new jpeg_decompress_struct();
	struct jpeg_decompress_struct& cinfo = *pTypedPayload->pDecompressStruct;

	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, pFile);
	jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);

	// this can't really fail in stdio_src mode apparently...
	jpeg_read_header(&cinfo, FALSE); // we don't need the image...

	// as we only want the header metadata, we don't need to call jpeg_start_decompress() at all...

	jpeg_saved_marker_ptr pMarker = cinfo.marker_list;

	bool found = false;

	while (pMarker != nullptr)
	{
		if (pMarker->marker == 0xe1) // 225
		{
			// we only care about the EXIF one for the moment...
			if (memcmp(pMarker->data, "Exif", 4) == 0)
			{
				exifData.pData = pMarker->data;
				exifData.length = pMarker->data_length;
				found = true;
				break;
			}
		}
		pMarker = pMarker->next;
	}

	fclose(pFile);

	return found;
}



Image3f* ImageReaderJPEG::readColour3fImage(const std::string& filePath) const
{
	FILE* pFile = fopen(filePath.c_str(), "rb");
	if (!pFile)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		return nullptr;
	}

	struct jpeg_decompress_struct cinfo;

	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, pFile);
	jpeg_read_header(&cinfo, TRUE);
	if (jpeg_start_decompress(&cinfo) != TRUE)
	{
		fprintf(stderr, "Can't open file: %s\n", filePath.c_str());
		fclose(pFile);
		jpeg_destroy_decompress(&cinfo);
		return nullptr;
	}

	int depth = cinfo.output_components;

	unsigned int width = cinfo.output_width;
	unsigned int height = cinfo.output_height;

	unsigned char** pScanlines = new unsigned char*[height];

	if (!pScanlines)
	{
		fprintf(stderr, "Cannot allocate memory to read file: %s\n", filePath.c_str());
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		fclose(pFile);
		return nullptr;
	}

	Image3f* pImage3f = new Image3f(width, height);
	if (!pImage3f)
	{
		fprintf(stderr, "Can't allocate memory for image...\n");
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		fclose(pFile);

		delete [] pScanlines;

		return nullptr;
	}

	// read all the scanlines
	for (unsigned int i = 0; i < height; i++)
	{
		// TODO: this could fail too...
		pScanlines[i] = new unsigned char[width * depth];
	}

	unsigned int linesRead = 0;

	while (linesRead < height)
		linesRead += jpeg_read_scanlines(&cinfo, &pScanlines[linesRead], height - linesRead);

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	unsigned char* pScanlineBuffer;

	for (unsigned int i = 0; i < height; i++)
	{
		pScanlineBuffer = pScanlines[i];
		// need to flip the height round...
		unsigned int y = height - i - 1;

		Colour3f* pImageRow = pImage3f->getRowPtr(y);

		if (depth == 3)
		{
			for (unsigned int x = 0; x < width; x++)
			{
				unsigned char red = *pScanlineBuffer++;
				unsigned char green = *pScanlineBuffer++;
				unsigned char blue = *pScanlineBuffer++;

				pImageRow->r = ColourSpace::convertSRGBToLinearLUT(red);
				pImageRow->g = ColourSpace::convertSRGBToLinearLUT(green);
				pImageRow->b = ColourSpace::convertSRGBToLinearLUT(blue);

				pImageRow++;
			}
		}
	}

	for (unsigned i = 0; i < height; i++)
	{
		unsigned char* pLine = pScanlines[i];
		delete [] pLine;
	}

	delete [] pScanlines;

	fclose(pFile);

	return pImage3f;
}

namespace
{
	ImageReader* createImageReaderJPEG()
	{
		return new ImageReaderJPEG();
	}

	const bool registered = FileIORegistry::instance().registerImageReaderMultipleExtensions("jpg;jpeg", createImageReaderJPEG);
}
