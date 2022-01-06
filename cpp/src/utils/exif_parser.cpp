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

#include "exif_parser.h"

#include <cstdio>

#include "thirdparty/exif.h"


EXIFParser::EXIFParser()
{

}

bool EXIFParser::readEXIFFromJPEGFile(const std::string& jpegFile, EXIFInfoBasic& exifInfo)
{
	FILE* fp = fopen(jpegFile.c_str(), "rb");
	if (!fp)
	{
		return false;
	}

	fseek(fp, 0, SEEK_END);
	unsigned long fsize = ftell(fp);
	rewind(fp);
	unsigned char* buf = new unsigned char[fsize];
	if (fread(buf, 1, fsize, fp) != fsize)
	{
		fclose(fp);
		delete[] buf;
		return false;
	}
	fclose(fp);

	easyexif::EXIFInfo result;
	int code = result.parseFrom(buf, fsize);
	delete [] buf;
	if (code)
	{
		return false;
	}

	extractEXIFInfo(result, exifInfo);

	return true;
}

bool EXIFParser::readEXIFFromMemory(const unsigned char* pMem, unsigned int memLength, EXIFInfoBasic& exifInfo)
{
	easyexif::EXIFInfo result;
	int code = result.parseFromEXIFSegment(pMem, memLength);
	if (code)
		return false;

	extractEXIFInfo(result, exifInfo);

	return true;
}

void EXIFParser::extractEXIFInfo(const easyexif::EXIFInfo& srcInfo, EXIFInfoBasic& finalInfo)
{
	finalInfo.m_cameraMake = srcInfo.Make;
	finalInfo.m_cameraModel = srcInfo.Model;

	// Note: DateTimeOriginal is sometimes set, but always matches Digitized if set, and sometimes misses seconds..
	finalInfo.m_takenDateTime = srcInfo.DateTimeDigitized;

/*
	fprintf(stderr, "File Info:\n");
	fprintf(stderr, "size: %u, %u\nCamera: %s\n", srcInfo.ImageWidth, srcInfo.ImageHeight, srcInfo.Model.c_str());
	fprintf(stderr, "Date: %s, %s\n\n", srcInfo.DateTimeDigitized.c_str(), srcInfo.DateTimeOriginal.c_str());
*/
}
