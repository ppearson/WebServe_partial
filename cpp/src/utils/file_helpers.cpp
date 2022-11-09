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

#include "file_helpers.h"

#include <cstdio>
#include <cstring>

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <set>
#include <algorithm>

FileHelpers::FileHelpers()
{

}

std::string FileHelpers::getFileExtension(const std::string& path)
{
	size_t dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos)
		return "";

	std::string extension = path.substr(dotPos + 1);

	std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

	return extension;
}

std::string FileHelpers::getFileNameStem(const std::string& fileNamePath)
{
	// TODO: make this cope with a full path with directory as well...

	size_t dotPos = fileNamePath.find_last_of('.');
	if (dotPos == std::string::npos)
		return "";

	std::string stem = fileNamePath.substr(0, dotPos);
	return stem;
}

std::string FileHelpers::getFileDirectory(const std::string& path)
{
	std::string directory;
	size_t slashPos = path.find_last_of('/', path.length());
	if (slashPos != std::string::npos)
		directory = path.substr(0, slashPos + 1);

	return directory;
}

std::string FileHelpers::getFileName(const std::string& path)
{
	std::string fileName;
	size_t slashPos = path.find_last_of('/', path.length());
	if (slashPos != std::string::npos)
		fileName = path.substr(slashPos + 1);
	else
		return path;

	return fileName;
}

std::string FileHelpers::combinePaths(const std::string& path0, const std::string& path1)
{
	if (path0.empty())
		return path1;

	std::string final = path0;

	if (strcmp(final.substr(final.size() - 1, 1).c_str(), "/") != 0)
	{
		final += "/";
	}

	final += path1;

	return final;
}

bool FileHelpers::removePrefixFromPath(std::string& path, const std::string& prefixPath)
{
	if (path.substr(0, prefixPath.size()) == prefixPath)
	{
		// the path begins with the prefix, so just remove it

		path = path.substr(prefixPath.size());
		return true;
	}

	return false;
}

bool FileHelpers::getFilesInDirectory(const std::string& directoryPath, const std::string& extension, std::vector<std::string>& files, bool recursive)
{
	// Note: opendir() is used on purpose here, as scandir() and lsstat() don't reliably support S_ISLNK on symlinks over NFS,
	//       whereas opendir() allows this
	DIR* dir = opendir(directoryPath.c_str());
	if (!dir)
		return false;

	struct dirent* dirEnt = nullptr;
	char tempBuffer[4096];

	while ((dirEnt = readdir(dir)) != nullptr)
	{
		if (dirEnt->d_type == DT_DIR)
		{
			if (!recursive)
			{
				continue;
			}

			if (std::strcmp(dirEnt->d_name, ".") == 0 || std::strcmp(dirEnt->d_name, "..") == 0)
			{
				continue;
			}

			// otherwise, recurse...
			std::string subDirPath = combinePaths(directoryPath, dirEnt->d_name);
			// TODO: recursion depth limit?
			getFilesInDirectory(subDirPath, extension, files, true);
		}

		// cope with symlinks by working out what they point at
		if (dirEnt->d_type == DT_LNK)
		{
			std::string fullAbsolutePath = combinePaths(directoryPath, dirEnt->d_name);
			if (readlink(fullAbsolutePath.c_str(), tempBuffer, 4096) == -1)
			{
				// something went wrong, so ignore...
				continue;
			}
			else
			{
				// on the assumption that the target of the symlink is not another symlink (if so, this won't work over NFS)
				// check what type it is
				struct stat statState;
				int ret = lstat(tempBuffer, &statState);

				if (ret == -1 || S_ISDIR(statState.st_mode))
				{
					// ignore for the moment...
					continue;
				}
				else
				{
					// TODO: do we need to check this is a full absolute path?
					// TODO: do de-duplication on target, as opposed to original symlink in dir listing

					if (getFileExtension(dirEnt->d_name) == extension)
					{
						files.push_back(fullAbsolutePath);
					}
				}
			}
		}
		else
		{
			// it's hopefully a file
			std::string fullAbsolutePath = combinePaths(directoryPath, dirEnt->d_name);
			if (getFileExtension(dirEnt->d_name) == extension)
			{
				files.push_back(fullAbsolutePath);
			}
		}
	}

	closedir(dir);

	return !files.empty();
}

bool FileHelpers::getRelativeFilesInDirectoryRecursive(const std::string& searchDirectoryPath, const std::string& relativeDirectoryPath,
													   const std::string& extension, std::vector<std::string>& files)
{
	// TODO: support symlinks...

	DIR* dir = opendir(searchDirectoryPath.c_str());
	if (!dir)
		return false;

	struct dirent* dirEnt = nullptr;

	while ((dirEnt = readdir(dir)) != nullptr)
	{
		if (dirEnt->d_type == DT_DIR)
		{
			// ignore built-in items
			if (strcmp(dirEnt->d_name, ".") == 0 || strcmp(dirEnt->d_name, "..") == 0)
				continue;

			// build up next directory level relative path
			std::string newFullDirPath = combinePaths(searchDirectoryPath, dirEnt->d_name);
			std::string newRelativeDirPath = combinePaths(relativeDirectoryPath, dirEnt->d_name);
			getRelativeFilesInDirectoryRecursive(newFullDirPath, newRelativeDirPath, extension, files);
		}

		// cope with symlinks by working out what they point at
		if (dirEnt->d_type == DT_LNK)
		{
			// TODO: implement...
			continue;
		}
		else
		{
			// it's hopefully a file
			std::string fullRelativePath = combinePaths(relativeDirectoryPath, dirEnt->d_name);
			if (getFileExtension(dirEnt->d_name) == extension)
			{
				files.push_back(fullRelativePath);
			}
		}
	}

	closedir(dir);

	return !files.empty();
}

bool FileHelpers::checkFileExists(const std::string& filePath)
{
	FILE* file = fopen(filePath.c_str(), "r");
	if (file == nullptr)
		return false;

	fclose(file);
	return true;
}

bool FileHelpers::checkDirectoryExists(const std::string& dirPath)
{
	DIR* dir = opendir(dirPath.c_str());
	if (!dir)
		return false;

	closedir(dir);

	return true;
}

bool FileHelpers::createDirectory(const std::string& dirPath)
{
	DIR* dir = opendir(dirPath.c_str());
	if (dir)
	{
		// already exists
		closedir(dir);
		return true;
	}
	
	// this only seems to create a final missing directory entry, so if we want it to do more
	// we'll have to walk back the items manually creating new directories one at a time..
	int status = mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	
	return status == 0;
}

std::string FileHelpers::getFileTextContent(const std::string& filePath)
{
	FILE* pFile = fopen(filePath.c_str(), "rb");
	if (!pFile)
	{
	   return "";
	}

	if (fseek(pFile, 0, SEEK_END) < 0)
	{
		fclose(pFile);
		return "";
	}

	const long int fileSize = ftell(pFile);
	if (fileSize < 0)
	{
		fclose(pFile);
		return "";
	}

	rewind(pFile);

	char* tempBuffer = (char*)malloc(fileSize + 1);
	if (!tempBuffer)
	{
		fclose(pFile);
		return "";
	}

	if (fread(tempBuffer, sizeof(char), fileSize, pFile) < fileSize)
	{
		free(tempBuffer);
		fclose(pFile);
		return "";
	}

	tempBuffer[fileSize] = '\0';

	std::string fileContent(tempBuffer);

	free(tempBuffer);

	fclose(pFile);

	return fileContent;
}
