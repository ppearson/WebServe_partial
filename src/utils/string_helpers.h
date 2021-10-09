/*
 WebServe
 Copyright 2018-2019 Peter Pearson.

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

#ifndef STRING_HELPERS_H
#define STRING_HELPERS_H

#include <string>
#include <vector>

class StringHelpers
{
public:
	StringHelpers();

	static void split(const std::string& str, std::vector<std::string>& tokens, const std::string& sep = "\n");
	static bool splitInTwo(const std::string& str, std::string& value0, std::string& value1, const std::string& sep = ":");
	static std::string toLower(const std::string& str);
	static void toLowerInPlace(std::string& str);

	// designed for use when comparing subsections of std::string& against a static const char*, which if done naively,
	// will create a temporary std::string for comparison purposes. TODO: string_view should help here?
	// However, it seems that when operating directly on an std::string, the executable size is no different
	// between using this vs doing str.substr() ==, so maybe this isn't needed from an efficiency point-of-view...
	static bool beginsWithStaticConst(const std::string& str, const char* prefix);
	static bool endsWithStaticConst(const std::string& str, const char* postfix);

	static bool isNumber(const std::string& str);

	static void stripWhitespace(std::string& str);

	static void getSetTokensFromString(const std::string& str, std::vector<std::string>& tokens);
	static std::string combineSetTokens(const std::string& str1, const std::string& str2);

	// do basic URL / %-encoding
	static std::string simpleEncodeString(const std::string& inputString);
	static std::string simpleDecodeString(const std::string& inputString);

	static std::string base64Encode(const std::string& inputString);
	static std::string base64Decode(const std::string& inputString);

	static std::string generateRandomASCIIString(unsigned int length);
	
	static std::string formatSize(size_t amount);
	static std::string formatNumberThousandsSeparator(size_t value);
	
	static std::string formatTimePeriod(double seconds, bool keepAsSeconds = false);
};

#endif // STRING_HELPERS_H
