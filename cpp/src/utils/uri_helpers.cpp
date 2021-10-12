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

#include "uri_helpers.h"

#include <algorithm>

URIHelpers::URIHelpers()
{

}

bool URIHelpers::splitFirstLevelDirectoryAndRemainder(const std::string& uri, std::string& directory, std::string& remainder)
{
	size_t splitPos = uri.find('/');

	if (splitPos == std::string::npos)
		return false;

	directory = uri.substr(0, splitPos);
	remainder = uri.substr(splitPos + 1);

	return true;
}

std::string URIHelpers::getFileExtension(const std::string& uri)
{
	size_t dotPos = uri.find_last_of('.');
	if (dotPos == std::string::npos)
		return "";

	std::string extension = uri.substr(dotPos + 1);

	std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

	return extension;
}
