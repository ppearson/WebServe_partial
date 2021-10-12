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

#include "string_helpers.h"

#include <ctime>
#include <cstdlib>
#include <cstring> // for memset
#include <algorithm>

static const std::string kBase64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const std::string kTokenSetSeparator = ",";

StringHelpers::StringHelpers()
{

}

void StringHelpers::split(const std::string& str, std::vector<std::string>& tokens, const std::string& sep)
{
	int lastPos = str.find_first_not_of(sep, 0);
	int pos = str.find_first_of(sep, lastPos);

	while (lastPos != -1 || pos != -1)
	{
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		lastPos = str.find_first_not_of(sep, pos);
		pos = str.find_first_of(sep, lastPos);
	}
}

bool StringHelpers::splitInTwo(const std::string& str, std::string& value0, std::string& value1, const std::string& sep)
{
	size_t sepPos = str.find(sep);

	if (sepPos == std::string::npos)
		return false;

	value0 = str.substr(0, sepPos);
	value1 = str.substr(sepPos + 1);

	return true;
}

std::string StringHelpers::toLower(const std::string& str)
{
	std::string result;
	result.reserve(str.size());

	unsigned int size = str.size();
	for (unsigned int i = 0; i < size; i++)
		result += tolower(str[i]);

	return result;
}

void StringHelpers::toLowerInPlace(std::string& str)
{
	unsigned int size = str.size();
	for (unsigned int i = 0; i < size; i++)
		str[i] = tolower(str[i]);
}

bool StringHelpers::beginsWithStaticConst(const std::string& str, const char* prefix)
{
	// hopefully the compiler can work out the strlen() of const static strings at compile time?
	return !str.empty() && (strncmp(str.c_str(), prefix, strlen(prefix)) == 0);
}

bool StringHelpers::endsWithStaticConst(const std::string& str, const char* postfix)
{
	if (str.empty())
		return false;

	unsigned int lengthPostFix = strlen(postfix);
	if (str.size() < lengthPostFix)
		return false;

	const char* pCompareStartPos = str.c_str() + (str.size() - lengthPostFix);
	return (strncmp(pCompareStartPos, postfix, lengthPostFix) == 0);
}

bool StringHelpers::isNumber(const std::string& str)
{
	unsigned int size = str.size();
	for (unsigned int i = 0; i < size; i++)
	{
		if (!std::isdigit(str[i]))
			return false;
	}

	return size > 0;
}

void StringHelpers::stripWhitespace(std::string& str)
{
	if (str.empty())
		return;

	size_t firstNonWhitespacePos = str.find_first_not_of(' ', 0);
	if (firstNonWhitespacePos == std::string::npos)
	{
		// we didn't find a non-whitespace char, so set string to empty
		str = "";
		return;
	}

	// strip the front off before looking for the trailing whitespace
	// TODO: this could all be done in one pass
	str = str.substr(firstNonWhitespacePos);

	// do the end strip manually, as c++ doesn't have anything useful to do this for us.
	size_t lastGoodChar = std::string::npos;
	for (unsigned int i = str.size(); i > 1; i--)
	{
		unsigned int checkCharPos = i - 1;
		if (str[checkCharPos] != ' ')
		{
			lastGoodChar = checkCharPos;
			break;
		}
	}

	if (lastGoodChar != std::string::npos)
	{
		str = str.substr(0, lastGoodChar + 1);
	}
}

void StringHelpers::getSetTokensFromString(const std::string& str, std::vector<std::string>& tokens)
{
	std::vector<std::string> tempTokens;
	split(str, tempTokens, kTokenSetSeparator);

	for (std::string strItem : tempTokens)
	{
		stripWhitespace(strItem);

		if (!strItem.empty())
		{
			tokens.emplace_back(strItem);
		}
	}
}

std::string StringHelpers::combineSetTokens(const std::string& str1, const std::string& str2)
{
	// for the moment, just append the two strings together, making sure there's a seperating token seperator char between them,
	// but this is likely going to require being made more robust in the future...

	if (str1.empty())
		return str2;

	if (str2.empty())
		return str1;

	std::string combinedString = str1;

	if (combinedString.substr(combinedString.size() - 1, 1) != kTokenSetSeparator)
	{
		combinedString += kTokenSetSeparator + " ";
	}

	combinedString += str2;

	return combinedString;
}

std::string StringHelpers::simpleEncodeString(const std::string& inputString)
{
	std::string outputString;

	for (char c : inputString)
	{
		if (c == ' ')
		{
			outputString += "+";
		}
		else if (c == '/')
		{
			outputString += "%2F";
		}
		else
		{
			outputString += c;
		}
	}

	return outputString;
}

std::string StringHelpers::simpleDecodeString(const std::string& inputString)
{
	// convert hex encoded strings to native strings
	std::string output = inputString;

	size_t nHex = 0;
	while ((nHex = output.find('%', nHex)) != std::string::npos)
	{
		std::string strHex = "0x" + output.substr(nHex + 1, 2);
		char cChar = static_cast<char>(strtol(strHex.c_str(), nullptr, 16));
		char szTemp[2];
		memset(szTemp, 0, 2);
		sprintf(szTemp, "%c", cChar);
		std::string strChar(szTemp);

		output.replace(nHex, 3, strChar);
	}

	// + to spaces

	size_t nSpace = 0;
	while ((nSpace = output.find('+', nSpace)) != std::string::npos)
	{
		output.replace(nSpace, 1, " ");
	}

	return output;
}

std::string StringHelpers::base64Encode(const std::string& inputString)
{
	std::string outputString;

	int val0 = 0;
	int val1 = -6;

	for (unsigned int i = 0; i < inputString.size(); i++)
	{
		const char& c = inputString[i];

		val0 = (val0 << 8) + c;
		val1 += 8;
		while (val1 >= 0)
		{
			outputString.push_back(kBase64Chars[(val0 >> val1) & 0x3F]);
			val1 -= 6;
		}
	}

	if (val1 > -6)
	{
		outputString.push_back(kBase64Chars[((val0 << 8) >> (val1 + 8)) & 0x3F]);
	}

	while (outputString.size() % 4)
	{
		outputString += "=";
	}

	return outputString;
}

std::string StringHelpers::base64Decode(const std::string& inputString)
{
	std::string outputString;

	// TODO: doing this each time is obviously inefficient...
	std::vector<int> temp(256, -1);
	for (unsigned int i = 0; i < 64; i++)
	{
		temp[kBase64Chars[i]] = i;
	}

	int val0 = 0;
	int val1 = -8;

	for (unsigned int i = 0; i < inputString.size(); i++)
	{
		const char& c = inputString[i];

		if (temp[c] == -1)
			break;

		val0 = (val0 << 6) + temp[c];
		val1 += 6;
		if (val1 >= 0)
		{
			outputString.push_back((char)(val0 >> val1) & 0xFF);
			val1 -= 8;
		}
	}

	return outputString;
}

std::string StringHelpers::generateRandomASCIIString(unsigned int length)
{
	// TODO: something a lot better than this in terms of security...
	srand(time(nullptr));

	char szRandomString[32];
	memset(szRandomString, 0, 32);

	for (unsigned int i = 0; i < length; i++)
	{
		unsigned int index = rand() % 56;
		szRandomString[i] = kBase64Chars[index] ;
	}

	return std::string(szRandomString);
}

std::string StringHelpers::formatSize(size_t amount)
{
	char szMemAvailable[16];
	std::string units;
	unsigned int size = 0;
	char szDecimalSize[12];
	if (amount >= 1024 * 1024 * 1024) // GB
	{
		size = amount / (1024 * 1024);
		float fSize = (float)size / 1024.0f;
		sprintf(szDecimalSize, "%.3f", fSize);
		units = "GB";
	}
	else if (amount >= 1024 * 1024) // MB
	{
		size = amount / 1024;
		float fSize = (float)size / 1024.0f;
		sprintf(szDecimalSize, "%.3f", fSize);
		units = "MB";
	}
	else if (amount >= 1024) // KB
	{
		size = amount;
		float fSize = (float)size / 1024.0f;
		sprintf(szDecimalSize, "%.1f", fSize);
		units = "KB";
	}
	else
	{
		sprintf(szDecimalSize, "0");
		units = "B"; // just so it makes sense
	}

	sprintf(szMemAvailable, "%s %s", szDecimalSize, units.c_str());
	std::string final(szMemAvailable);
	return final;
}

std::string StringHelpers::formatNumberThousandsSeparator(size_t value)
{
	char szRawNumber[32];
	sprintf(szRawNumber, "%zu", value);

	std::string temp(szRawNumber);

	std::string final;
	int i = temp.size() - 1;
	unsigned int count = 0;
	for (; i >= 0; i--)
	{
		final += temp[i];

		if (count++ == 2 && i != 0)
		{
			final += ",";
			count = 0;
		}
	}

	std::reverse(final.begin(), final.end());

	return final;
}

std::string StringHelpers::formatTimePeriod(double seconds, bool keepAsSeconds)
{
	char szTemp[32];
	if (keepAsSeconds)
	{
		sprintf(szTemp, "%0.4f s", seconds);
		return std::string(szTemp);
	}

	if (seconds < 60.0)
	{
		sprintf(szTemp, "00:%02.f m", seconds);
		return std::string(szTemp);
	}

	unsigned int minutes = (unsigned int)(seconds / 60.0);
	seconds -= minutes * 60;

	// we need to cater for seconds now being something like 59.98999,
	// which when printed out with no decimal places becomes 60, which while
	// technically correct, looks wrong.
	// TODO: a better way of doing this? Just round it as an int?
	if (seconds >= 59.5)
	{
		minutes += 1.0f;
		seconds = 0;
	}

	if (minutes < 60)
	{
		sprintf(szTemp, "%02d:%02.f m", minutes, seconds);
		return std::string(szTemp);
	}

	unsigned int hours = (unsigned int)(minutes / 60.0);
	minutes -= hours * 60;
	
	if (hours <= 23)
	{
		sprintf(szTemp, "%u:%02d:%02.f h", hours, minutes, seconds);
		return std::string(szTemp);
	}
	
	unsigned int days = (unsigned int)(hours / 24.0);
	hours -= days * 24;
	
	if (days == 1)
	{
		sprintf(szTemp, "1 day, %u:%02d:%02.f h", hours, minutes, seconds);
	}
	else
	{
		sprintf(szTemp, "%u days, %u:%02d:%02.f h", days, hours, minutes, seconds);
	}
	return std::string(szTemp);
}
