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

#ifndef FILE_HELPERS_H
#define FILE_HELPERS_H

#include <string>
#include <vector>

class FileHelpers
{
public:
	FileHelpers();


	static std::string getFileExtension(const std::string& path);
	// get filename without extension
	static std::string getFileNameStem(const std::string& fileNamePath);

	static std::string getFileDirectory(const std::string& path);
	static std::string getFileName(const std::string& path);

	static std::string combinePaths(const std::string& path0, const std::string& path1);

	// this can make paths relative by removing a prefix part
	static bool removePrefixFromPath(std::string& path, const std::string& prefixPath);

	static bool getFilesInDirectory(const std::string& directoryPath, const std::string& extension, std::vector<std::string>& files, bool recursive = false);

	static bool getRelativeFilesInDirectoryRecursive(const std::string& searchDirectoryPath, const std::string& relativeDirectoryPath,
													 const std::string& extension, std::vector<std::string>& files);

	static bool checkFileExists(const std::string& filePath);
	static bool checkDirectoryExists(const std::string& dirPath);
	
	static bool createDirectory(const std::string& dirPath);
	
	static std::string getFileTextContent(const std::string& filePath);
};

#endif // FILE_HELPERS_H
