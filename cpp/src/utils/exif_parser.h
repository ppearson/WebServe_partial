/*
 WebServe
 Copyright 2018 Peter Pearson.

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

#ifndef EXIF_PARSER_H
#define EXIF_PARSER_H

#include <string>

// wrapper around third-party EXIF parser class
// while this external class works, it's not perfect (needs memory allocated to read exif data which is a bit annoying),
// so long-term it'd be worth writing our own logic to handle this more efficiently.

// TODO: use libexif instead on Linux?/as well (on OS X we'll need something to fall back on by default...)

namespace easyexif
{
	class EXIFInfo;
}

struct EXIFInfoBasic
{
	EXIFInfoBasic() : m_width(0),
		m_height(0)
	{

	}

	unsigned int	m_width;
	unsigned int	m_height;

	std::string		m_takenDateTime;

	std::string		m_cameraMake;
	std::string		m_cameraModel;
};

class EXIFParser
{
public:
	EXIFParser();

	static bool readEXIFFromJPEGFile(const std::string& jpegFile, EXIFInfoBasic& exifInfo);

	static bool readEXIFFromMemory(const unsigned char* pMem, unsigned int memLength, EXIFInfoBasic& exifInfo);

protected:
	static void extractEXIFInfo(const easyexif::EXIFInfo& srcInfo, EXIFInfoBasic& finalInfo);
};

#endif // EXIF_PARSER_H
