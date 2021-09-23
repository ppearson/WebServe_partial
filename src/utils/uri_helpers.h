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

#ifndef URI_HELPERS_H
#define URI_HELPERS_H

#include <string>

class URIHelpers
{
public:
	URIHelpers();

	// The assumption here is that all URIs are relative (i.e. there's no leading slash)
	static bool splitFirstLevelDirectoryAndRemainder(const std::string& uri, std::string& directory, std::string& remainder);

	static std::string getFileExtension(const std::string& uri);
};

#endif // URI_HELPERS_H
