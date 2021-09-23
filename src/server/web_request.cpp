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

#include "web_request.h"

#include <sstream>
#include <iostream>
#include <vector>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "utils/string_helpers.h"

WebRequest::WebRequest(const std::string& rawRequest) : m_rawRequest(rawRequest),
	m_requestType(eRequestUnknown),
    m_httpVersion(eHTTPUnknown),
	m_connectionType(eConnectionUnknown),
	m_fileType(eFTUnknown),
	m_headerAuthenticationType(eAuthNone)
{

}

bool WebRequest::parse(Logger& logger)
{
	// TODO: doing it this way is very hacky as it means we lose blank lines (end of header, etc)...
	// TODO: also, allocating loads of separate strings for each line is fairly expensive - use some stringView type of thing?
	std::vector<std::string> lines;
	StringHelpers::split(m_rawRequest, lines);

	if (lines.empty())
		return false;

	// first line should contain main request
	const std::string &line = lines[0];

	size_t httpVersionItemStart = line.find("HTTP/");
	if (httpVersionItemStart == std::string::npos)
		return false;

	std::string httpVersionString = line.substr(httpVersionItemStart + 5, 3);
	if (httpVersionString == "1.1")
	{
		m_httpVersion = eHTTP11;
	}
	else if (httpVersionString == "1.0")
	{
		m_httpVersion = eHTTP10;
	}
	else if (httpVersionString == "0.9")
	{
		m_httpVersion = eHTTP09;
	}

	if (!StringHelpers::beginsWithStaticConst(line, "GET"))
	{
		if (!StringHelpers::beginsWithStaticConst(line, "POST"))
		{
			if (!StringHelpers::beginsWithStaticConst(line, "HEAD"))
			{
				std::string command;
				size_t commandSep = line.find(' ');
				if (commandSep != std::string::npos)
				{
					command = line.substr(0, commandSep);
				}
				
				m_requestType = eRequestUnknown;
	
				logger.error("Unsupported HTTP command in request from client: %s", command.c_str());
			}
			else
			{
				m_requestType = eRequestHEAD;
				
				// we recognise this, but don't handle it (for the moment)
			}
			
			return false;
		}

		m_requestType = eRequestPOST;
	}
	else
	{
		// it was a
		m_requestType = eRequestGET;		
	}

	size_t pathStart = m_requestType != eRequestGET ? 5 : 4;

	size_t pathEnd = line.rfind(' '); // should be the space before the HTTP version

	if (pathEnd == std::string::npos)
		return false;

	const std::string path = line.substr(pathStart, pathEnd - pathStart);

	size_t nQuestionMark = path.find('?');
	if (nQuestionMark == std::string::npos)
	{
		m_path = path;
	}
	else
	{
		m_path = path.substr(0, nQuestionMark);

		// if POST has this on the end, ignore the URL params
		if (m_requestType != eRequestPOST)
		{
			std::string params = path.substr(nQuestionMark + 1);

			addParams(params);
		}
	}

	// try and work out the file type
	if (m_requestType != eRequestPOST)
	{
		size_t extensionSep = m_path.find_last_of('.');
		if (extensionSep != std::string::npos)
		{
			std::string extensionString = m_path.substr(extensionSep + 1);
			if (extensionString == "css")
			{
				m_fileType = eFTCSS;
			}
		}
	}

	std::vector<std::string>::const_iterator itLine = lines.begin();
	++ itLine; // skip the first line which we've processed already


	bool lookForAuthentication = false;
	bool foundCookie = false;
	bool foundUserAgent = false;
	bool foundHost = false;
	bool foundConnection = false;

	// TODO: this needs to cope with case-insensitive comparisons...
	//       We can't just make the entire line lower-case though, it needs to be itemised

	for (; itLine != lines.end(); ++itLine)
	{
		const std::string& otherLine = *itLine;

		// now try and find an authentication line...
		if (lookForAuthentication && (otherLine.compare(0, 14, "Authorization:") == 0))
		{
			std::string authorizationString = otherLine.substr(15);

			processAuthenticationHeader(authorizationString);
			lookForAuthentication = false;
		}
		else if (!foundCookie && (otherLine.compare(0, 7, "Cookie:") == 0))
		{
			std::string cookieString;
			cookieString = extractFieldItem(otherLine, 7 + 1);

			processCookieHeader(cookieString);
			foundCookie = true;
		}
		else if (!foundUserAgent && (otherLine.compare(0, 11, "User-Agent:") == 0))
		{
			m_userAgentField = extractFieldItem(otherLine, 11 + 1);

			foundUserAgent = true;
		}
		else if (!foundHost && (otherLine.compare(0, 5, "Host:") == 0))
		{
			m_hostValue = extractFieldItem(otherLine, 5 + 1);

			foundHost = true;
		}
		else if (!foundConnection && (otherLine.compare(0, 11, "Connection:") == 0))
		{
			m_connectionValue = extractFieldItem(otherLine, 11 + 1);

			foundConnection = true;
		}
	}

	// apply defaults
	if (m_httpVersion == eHTTP11)
	{
		m_connectionType = eConnectionKeepAlive;
	}
	else if (m_httpVersion == eHTTP09 || m_httpVersion == eHTTP10)
	{
		m_connectionType = eConnectionClose;
	}

	if (!m_connectionValue.empty() && (m_httpVersion == eHTTP10 || m_httpVersion == eHTTP11))
	{
		std::string lowerCaseConnectionValue = StringHelpers::toLower(m_connectionValue);
		if (lowerCaseConnectionValue == "close")
		{
			m_connectionType = eConnectionClose;
		}
		else if (lowerCaseConnectionValue == "keep-alive")
		{
			m_connectionType = eConnectionKeepAlive;
		}
		else
		{
			logger.warning("Unknown connection type specified: %s", m_connectionValue.c_str());
		}
	}

	// TODO: this is crap - we should check for the double CR and use that to detect the end of the header...
	//       or as a hacky stop-gap to be more robust (and detect missing POST data), match the line
	//       by the Content-Length....
	if (m_requestType == eRequestPOST && lines.size() > 1)
	{
		// hopefully, last line of HTTP request should be the POST params

		const std::string& params = lines.back();

		// Note: Sometimes we don't seem to have any POST data, as WebKit always seems to send POST webform
		//       data after the data start in a second packet and our single receive doesn't cope with this...

		addParams(params);
	}

	return true;
}

bool WebRequest::hasParam(const std::string &name) const
{
	std::map<std::string, std::string>::const_iterator itFind = m_aParams.find(name);

	return itFind != m_aParams.end();
}

std::string WebRequest::getParam(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator itFind = m_aParams.find(name);
	
	if (itFind == m_aParams.end())
		return "";

	return itFind->second;
}

int WebRequest::getParamAsInt(const std::string& name, int defaultVal) const
{
	std::map<std::string, std::string>::const_iterator itFind = m_aParams.find(name);
	
	if (itFind == m_aParams.end())
		return defaultVal;
	
	std::string paramValue = itFind->second;

	if (paramValue.empty())
	{
		return defaultVal;
	}

	return atoi(paramValue.c_str());
}

// Note: this rebuilds the params in alphabetical order, instead of the original
//       order they were in from the original request.
std::string WebRequest::getParamsAsGETString(bool ignorePaginationParams) const
{
	std::string paramString;
	
	for (const auto& itParam : m_aParams)
	{
		const std::string& paramName = itParam.first;
		const std::string& paramValue = itParam.second;
		
		if (ignorePaginationParams)
		{
			if (paramName == "perPage" || paramName == "startIndex")
				continue;
		}
		
		if (!paramString.empty())
			paramString += "&";
		
		paramString += paramName + "=";
		paramString += StringHelpers::simpleEncodeString(paramValue);
	}
	
	return paramString;
}

bool WebRequest::hasCookie(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator itFind = m_aCookies.find(name);

	return itFind != m_aCookies.end();
}

std::string WebRequest::getCookie(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator itFind = m_aCookies.find(name);

	if (itFind == m_aCookies.end())
		return "";

	return itFind->second;
}

int WebRequest::getCookieAsInt(const std::string& name, int defaultVal) const
{
	std::string cookieValue = getCookie(name);

	if (cookieValue.empty())
	{
		return defaultVal;
	}

	return atoi(cookieValue.c_str());
}

int WebRequest::getParamOrCookieAsInt(const std::string& paramName, const std::string& cookieName, int defaultValue) const
{
	std::map<std::string, std::string>::const_iterator itFind = m_aParams.find(paramName);

	if (itFind != m_aParams.end())
	{
		std::string paramValue = itFind->second;
		if (!paramValue.empty())
		{
			return atoi(paramValue.c_str());
		}
	}

	// otherwise, look for the cookie...
	std::string cookieValue = getCookie(cookieName);

	if (cookieValue.empty())
	{
		return defaultValue;
	}

	return atoi(cookieValue.c_str());
}

std::string WebRequest::extractFieldItem(const std::string& fieldString, unsigned int valueStartPos)
{
	std::string fieldValue;
	if (fieldString.substr(fieldString.size() - 1, 1) == "\r")
	{
		fieldValue = fieldString.substr(valueStartPos, fieldString.size() - 1 - valueStartPos);
	}
	else
	{
		fieldValue = fieldString.substr(valueStartPos);
	}

	return fieldValue;
}

void WebRequest::addParams(const std::string& params)
{
	std::vector<std::string> items;
	StringHelpers::split(params, items, "&");

	std::vector<std::string>::iterator it = items.begin();
	std::vector<std::string>::iterator itEnd = items.end();
	for (; it != itEnd; ++it)
	{
		const std::string& item = *it;

		size_t sep = item.find('=');
		if (sep == std::string::npos)
			continue;

		std::string name = item.substr(0, sep);
		std::string value = item.substr(sep + 1);

		// convert hex encoded strings to native strings

		size_t nHex = 0;
		while ((nHex = value.find('%', nHex)) != std::string::npos)
		{
			std::string strHex = "0x" + value.substr(nHex + 1, 2);
			char cChar = static_cast<char>(strtol(strHex.c_str(), nullptr, 16));
			char szTemp[2];
			memset(szTemp, 0, 2);
			sprintf(szTemp, "%c", cChar);
			std::string strChar(szTemp);

			value.replace(nHex, 3, strChar);
		}

		// + to spaces

		size_t nSpace = 0;
		while ((nSpace = value.find('+', nSpace)) != std::string::npos)
		{
			value.replace(nSpace, 1, " ");
		}

		if (!name.empty() && !value.empty())
		{
			m_aParams[name] = value;
		}
	}
}

void WebRequest::processAuthenticationHeader(const std::string& authorizationString)
{
	size_t sepPos = authorizationString.find(' ');
	if (sepPos == std::string::npos)
	{
		m_headerAuthenticationType = eAuthMalformed;
		return;
	}

	std::string authenticationType = authorizationString.substr(0, sepPos);

	if (authenticationType != "Basic")
	{
		m_headerAuthenticationType = eAuthUnknown;
		return;
	}

	m_headerAuthenticationType = eAuthBasic;

	std::string authorizationToken = authorizationString.substr(sepPos + 1);

	// remove any trailing "\r" char
	if (authorizationToken[authorizationToken.size() - 1] == '\r')
	{
		authorizationToken = authorizationToken.substr(0, authorizationToken.size() - 1);
	}

	authorizationToken = StringHelpers::base64Decode(authorizationToken);

	if (m_headerAuthenticationType == eAuthBasic)
	{
		size_t passSep = authorizationToken.find(':');
		if (passSep == std::string::npos)
		{
			m_headerAuthenticationType = eAuthMalformed;
			return;
		}

		std::string authUser = authorizationToken.substr(0, passSep);
		std::string authPass = authorizationToken.substr(passSep + 1);

		m_authUsername = authUser;
		m_authPassword = authPass;
	}
}

void WebRequest::processCookieHeader(const std::string& cookieString)
{
	std::vector<std::string> items;
	// TODO: this probably isn't completely robust to all types of strings, even if they are hex-encoded...
	StringHelpers::split(cookieString, items, "; ");

	for (const std::string& item : items)
	{
		size_t sep = item.find('=');
		if (sep == std::string::npos)
			continue;

		std::string name = item.substr(0, sep);
		std::string value = item.substr(sep + 1);

		if (!name.empty() && !value.empty())
		{
			m_aCookies[name] = value;
		}
	}
}
