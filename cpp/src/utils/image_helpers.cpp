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

#include "image_helpers.h"

#include <cstdio>

#include "utils/system.h"
#include "utils/file_helpers.h"

#include "io/file_io_registry.h"
#include "io/image_reader.h"

ImageHelpers::ImageHelpers()
{

}

bool ImageHelpers::getImageDimensionsCrap(const std::string& imagePath, uint16_t& width, uint16_t& height)
{
	FILE* pFile = fopen(imagePath.c_str(), "rb");
	if (!pFile)
		return false;

	bool foundMarker = false;
	unsigned char testByte;
	while (true)
	{
		if (!fread(&testByte, 1, sizeof(unsigned char), pFile))
			return false;

		if (testByte == 0xFF)
		{
			foundMarker = true;
			// found first bit

			continue;
		}
		if (foundMarker == true && testByte == 0xC0)
		{
			// We've found the SOF0 marker
			break;
		}
		else
		{
			foundMarker = false;
		}
	}

	uint16_t length;
	uint8_t bitsPerPixel;

	if (!fread(&length, 1, 2, pFile))
		return false;

	if (!fread(&bitsPerPixel, 1, 1, pFile))
		return false;

	if (!fread(&height, 1, 2, pFile))
		return false;

	if (!fread(&width, 1, 2, pFile))
		return false;

	width = System::convertToNetworkByteOrder(width);
	height = System::convertToNetworkByteOrder(height);

	fclose(pFile);

//	fprintf(stderr, "Size: %u, %u\n", width, height);

	return true;
}

bool ImageHelpers::getImageDimensions(const std::string& imagePath, unsigned int& width, unsigned int& height)
{
	std::string extension = FileHelpers::getFileExtension(imagePath);

	ImageReader* pImageReader = FileIORegistry::instance().createImageReaderForExtension(extension);

	if (!pImageReader)
		return false;

	ImageReader::ImageDetails details;
	if (!pImageReader->getImageDetails(imagePath, false, details))
	{
		delete pImageReader;
		return false;
	}

	width = details.width;
	height = details.height;

	delete pImageReader;
	return true;
}
