/*
 WebServe
 Copyright 2018 Peter Pearson.
 Originally taken from:
 Imagine
 Copyright 2011-2015 Peter Pearson.

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

#include "file_io_registry.h"

#include <vector>

#include "utils/string_helpers.h"

bool FileIORegistry::registerImageReader(const std::string& extension, CreateImageReaderCallback createReaderCB)
{
	m_imageReaders.insert(ImageReaderCallbacks::value_type(extension, createReaderCB));

	return true;
}

bool FileIORegistry::registerImageReaderMultipleExtensions(const std::string& extensions, CreateImageReaderCallback createReaderCB)
{
	std::vector<std::string> extensionsItems;

	StringHelpers::split(extensions, extensionsItems, ";");

	std::vector<std::string>::const_iterator itExt = extensionsItems.begin();
	for (; itExt != extensionsItems.end(); ++itExt)
	{
		const std::string& extension = *itExt;

		m_imageReaders.insert(ImageReaderCallbacks::value_type(extension, createReaderCB));
	}

	return true;
}

bool FileIORegistry::registerImageWriter(const std::string& extension, CreateImageWriterCallback createWriterCB)
{
	m_imageWriters.insert(ImageWriterCallbacks::value_type(extension, createWriterCB));

	return true;
}

ImageReader* FileIORegistry::createImageReaderForExtension(const std::string& extension) const
{
	ImageReaderCallbacks::const_iterator itFind = m_imageReaders.find(extension);
	if (itFind != m_imageReaders.end())
		return (itFind->second)();

	return nullptr;
}

ImageWriter* FileIORegistry::createImageWriterForExtension(const std::string& extension) const
{
	ImageWriterCallbacks::const_iterator itFind = m_imageWriters.find(extension);
	if (itFind != m_imageWriters.end())
		return (itFind->second)();

	return nullptr;
}
