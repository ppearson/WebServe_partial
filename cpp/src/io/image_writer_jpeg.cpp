/*
 WebServe
 Copyright 2018-2022 Peter Pearson.
 Originally taken from:
 Imagine
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

#include "image_writer_jpeg.h"

#include <cstdio>

#include <jpeglib.h>

#include "maths.h"
#include "image/image3f.h"

#include "image/colour_space.h"

// marker defines
#define ICC_JPEG_MARKER   JPEG_APP0 + 2

#define XMP_JPEG_MARKER   JPEG_APP0 + 1

ImageWriterJPEG::ImageWriterJPEG() : ImageWriter()
{
	
}

bool ImageWriterJPEG::writeImage(const std::string& filePath, const Image3f& image, const WriteParams& writeParams)
{
	unsigned int width = image.getWidth();
	unsigned int height = image.getHeight();
	
	struct jpeg_compress_struct compressInfo;
	struct jpeg_error_mgr jerr;
	
	FILE* pFile = fopen(filePath.c_str(), "wb");
	if (!pFile)
	{
		fprintf(stderr, "Error writing file: %s\n", filePath.c_str());
		return false;
	}
	
	jpeg_create_compress(&compressInfo);
	compressInfo.err = jpeg_std_error(&jerr);
		
	jpeg_stdio_dest(&compressInfo, pFile);
	
	compressInfo.image_width = width;
	compressInfo.image_height = height;
	compressInfo.input_components = 3;
	compressInfo.in_color_space = JCS_RGB;
	
	jpeg_set_defaults(&compressInfo);
	
	compressInfo.optimize_coding = TRUE;

	// disable progressive (for the moment anyway). jpeg_set_defaults() seems to do this anyway, but...
	compressInfo.scan_info = nullptr;
	compressInfo.num_scans = 0;
	
	// set actual options we want...
	int intQuality = (int)(writeParams.quality * 100.0f);
		
	jpeg_set_quality(&compressInfo, intQuality, TRUE);

	if (writeParams.chromaSubSamplingType == ImageWriter::eChromaSS_411)
	{
		// 0 = 4:1:1
		compressInfo.comp_info[0].h_samp_factor = 2;
		compressInfo.comp_info[0].v_samp_factor = 2;
	}
	else if (writeParams.chromaSubSamplingType == ImageWriter::eChromaSS_422)
	{
		// 1 = 4:2:2
		compressInfo.comp_info[0].h_samp_factor = 2;
		compressInfo.comp_info[0].v_samp_factor = 1;
	}
	else if (writeParams.chromaSubSamplingType == ImageWriter::eChromaSS_444)
	{
		// 2 = 4:4:4
		compressInfo.comp_info[0].h_samp_factor = 1;
		compressInfo.comp_info[0].v_samp_factor = 1;
	}
/*
	for (unsigned int i = 1; i < 3; i++)
	{
		compressInfo.comp_info[i].h_samp_factor = 1;
		compressInfo.comp_info[i].v_samp_factor = 1;
	}
*/
//	compressInfo.dct_method = JDCT_FLOAT;
	
	//

	//
	
	jpeg_start_compress(&compressInfo, TRUE);
	
	// allocate for only one scanline currently
	unsigned char* pTempRow = new unsigned char[width * 3];
	
	JSAMPROW rowPointer[1];
	
	for (unsigned int y = 0; y < height; y++)
	{
		// need to flip the height round...
		unsigned int actualRow = height - y - 1;
		const Colour3f* pRow = image.getRowPtr(actualRow);
		
		unsigned char* pTempScanline = pTempRow;

		for (unsigned int x = 0; x < width; x++)
		{
			float r = ColourSpace::convertLinearToSRGBAccurate(pRow->r);
			float g = ColourSpace::convertLinearToSRGBAccurate(pRow->g);
			float b = ColourSpace::convertLinearToSRGBAccurate(pRow->b);

			unsigned char red = MathsHelpers::clamp(r) * 255;
			unsigned char green = MathsHelpers::clamp(g) * 255;
			unsigned char blue = MathsHelpers::clamp(b) * 255;

			*pTempScanline++ = red;
			*pTempScanline++ = green;
			*pTempScanline++ = blue;

			pRow++;
		}
		
		rowPointer[0] = pTempRow;
		jpeg_write_scanlines(&compressInfo, rowPointer, 1);
	}
	
	jpeg_finish_compress(&compressInfo);
	
	delete [] pTempRow;
	
	fclose(pFile);
	
	jpeg_destroy_compress(&compressInfo);
	
	return true;
}

bool ImageWriterJPEG::writeRawImageCopy(const std::string& originalFilePath, const std::string& newFilePath, const WriteRawParams& params) const
{
	FILE* pSrcFile = fopen(originalFilePath.c_str(), "rb");
	if (!pSrcFile)
	{
		fprintf(stderr, "Can't open source file: %s\n", originalFilePath.c_str());
		return false;
	}

	struct jpeg_decompress_struct decompressInfo;

	struct jpeg_error_mgr jerr;
	decompressInfo.err = jpeg_std_error(&jerr);

	jpeg_create_decompress(&decompressInfo);
	jpeg_stdio_src(&decompressInfo, pSrcFile);
	
	jpeg_save_markers(&decompressInfo, JPEG_COM, 0xFFFF);
	for (unsigned i = 0; i < 16; i++) 
	{
		jpeg_save_markers(&decompressInfo, JPEG_APP0 + i, 0xFFFF);
	}
	
	jpeg_read_header(&decompressInfo, TRUE);
	
	struct jpeg_compress_struct compressInfo;
	
	FILE* pDstFile = fopen(newFilePath.c_str(), "wb");
	if (!pDstFile)
	{
		fprintf(stderr, "Error writing file: %s\n", newFilePath.c_str());
		return false;
	}
		
	jvirt_barray_ptr* pCoefArrays = jpeg_read_coefficients(&decompressInfo);
	if (!pCoefArrays)
	{
		return false;
	}
	
	jpeg_create_compress(&compressInfo);
	compressInfo.err = jpeg_std_error(&jerr);
	jpeg_stdio_dest(&compressInfo, pDstFile);
	
	jpeg_copy_critical_parameters(&decompressInfo, &compressInfo);
	compressInfo.optimize_coding = TRUE;

	// disable progressive (for the moment anyway). jpeg_set_defaults() seems to do this anyway, but...
	compressInfo.scan_info = nullptr;
	compressInfo.num_scans = 0;

	jpeg_write_coefficients(&compressInfo, pCoefArrays);

	copyRawMarkers(&decompressInfo, &compressInfo, params);

	jpeg_finish_compress(&compressInfo);
	
	jpeg_destroy_decompress(&decompressInfo);
	jpeg_destroy_compress(&compressInfo);

	fclose(pSrcFile);
	fclose(pDstFile);
	
	return true;
}

void ImageWriterJPEG::copyRawMarkers(struct jpeg_decompress_struct* pDInfo, struct jpeg_compress_struct* pCInfo, const WriteRawParams& params)
{
	if (!params.writeMetadata)
		return;
	
	jpeg_saved_marker_ptr marker = pDInfo->marker_list;
	
//	unsigned int markerCount = 0;
	
	while (marker)
	{
		bool shouldWriteMarker = true;
	/*	
		fprintf(stderr, "Marker: %u\tdata length: %u\n", markerCount++, marker->data_length);
		fprintf(stderr, "Data: %x %x %x %x %x\n", marker->data[0], marker->data[1], marker->data[2], marker->data[3], marker->data[4]);
		
		unsigned int dataSizeLeft = std::min(marker->data_length, 1024u);
		unsigned int dataOffset = 0;
		while (dataSizeLeft > 0)
		{
			unsigned int dataThisTime = std::min(dataSizeLeft, 128u);
			for (unsigned int i = 0; i < dataThisTime; i++)
			{
				fprintf(stderr, "%c", marker->data[dataOffset + i]);
			}
			
			dataSizeLeft -= dataThisTime;
			dataOffset += dataThisTime;
			fprintf(stderr, "\n");
		}
*/		
		// skip these ones, as libjpeg adds them again...
		// skip JFIF (APP0) marker
		if (marker->marker == JPEG_APP0 && marker->data_length >= 14 &&
				marker->data[0] == 0x4a &&
				marker->data[1] == 0x46 &&
				marker->data[2] == 0x49 &&
				marker->data[3] == 0x46 &&
				marker->data[4] == 0x00)
		{
			shouldWriteMarker = false;
		}
		
		if (marker->marker == JPEG_APP0 + 1)
		{
			shouldWriteMarker = true;
		}
	
		// skip Adobe (APP14) marker
		if (marker->marker == JPEG_APP0 + 14 && marker->data_length >= 12 &&
				marker->data[0] == 0x41 &&
				marker->data[1] == 0x64 &&
				marker->data[2] == 0x6f &&
				marker->data[3] == 0x62 &&
				marker->data[4] == 0x65)
		{
			shouldWriteMarker = false;
		}
		
		// skip ICC profile
		if (marker->marker == ICC_JPEG_MARKER &&
				marker->data[0] == 0x49 &&
				marker->data[1] == 0x43 &&
				marker->data[2] == 0x43 &&
				marker->data[3] == 0x5F &&
				marker->data[4] == 0x50)
		{
			shouldWriteMarker = false;
		}
		// skip XMP marker
		else if (marker->marker == XMP_JPEG_MARKER &&
				marker->data[0] == 0x68 &&
				marker->data[1] == 0x74 &&
				marker->data[2] == 0x74 &&
				marker->data[3] == 0x70 &&
				marker->data[4] == 0x3A)
		{
			if (params.stripXMPMetadata)
			{
				shouldWriteMarker = false;
			}
		}
		
		if (shouldWriteMarker)
		{
			jpeg_write_marker(pCInfo, marker->marker, marker->data, marker->data_length);
		}
		
		marker = marker->next;
	}
}

namespace
{
	ImageWriter* createImageWriterJPEG()
	{
		return new ImageWriterJPEG();
	}

	const bool registered = FileIORegistry::instance().registerImageWriter("jpg", createImageWriterJPEG);
}

