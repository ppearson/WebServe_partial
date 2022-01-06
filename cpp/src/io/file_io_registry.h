/*
 WebServe
 Copyright 2018-2022 Peter Pearson.
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

#ifndef FILE_IO_REGISTRY_H
#define FILE_IO_REGISTRY_H

#include <string>
#include <map>
#include <set>

class ImageReader;
class ImageWriter;

class FileIORegistry
{
public:
	static FileIORegistry& instance()
	{
		static FileIORegistry singleton;
		return singleton;
	}

	typedef ImageReader* (*CreateImageReaderCallback)();
	typedef ImageWriter* (*CreateImageWriterCallback)();

protected:
	typedef std::map<std::string, CreateImageReaderCallback>	ImageReaderCallbacks;
	typedef std::map<std::string, CreateImageWriterCallback>	ImageWriterCallbacks;

public:
	bool registerImageReader(const std::string& extension, CreateImageReaderCallback createReaderCB);
	// extensions separated by ";" char
	bool registerImageReaderMultipleExtensions(const std::string& extensions, CreateImageReaderCallback createReaderCB);
	bool registerImageWriter(const std::string& extension, CreateImageWriterCallback createWriterCB);

	// TODO: use unique_ptr or shared_ptr?
	ImageReader* createImageReaderForExtension(const std::string& extension) const;
	ImageWriter* createImageWriterForExtension(const std::string& extension) const;

protected:
	ImageReaderCallbacks		m_imageReaders;
	ImageWriterCallbacks		m_imageWriters;

private:
	FileIORegistry()
	{ }

	FileIORegistry(const FileIORegistry& vc);

	FileIORegistry& operator=(const FileIORegistry& vc);
};

#endif // FILE_IO_REGISTRY_H
