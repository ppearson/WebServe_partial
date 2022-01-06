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

#ifndef WEB_REQUEST_H
#define WEB_REQUEST_H

#include <string>
#include <map>

#include "utils/logger.h"

class WebRequest
{
public:
	WebRequest(const std::string& rawRequest);

	enum HTTPVersion
	{
		eHTTPUnknown,
		eHTTP09,
		eHTTP10,
		eHTTP11
	};
	
	enum HTTPRequestType
	{
		eRequestUnknown,
		eRequestGET,
		eRequestPOST,
		eRequestHEAD
	};

	enum AuthenticationType
	{
		eAuthNone,
		eAuthMalformed,
		eAuthUnknown,
		eAuthBasic,
		eAuthDigest
	};

	enum ConnectionType
	{
		eConnectionUnknown,
		eConnectionClose,
		eConnectionKeepAlive
	};

	enum FileType
	{
		eFTUnknown,
		eFTHTML,
		eFTCSS,
		eFTJS,
		eFTImage
	};

	// not great passing in logger like this, but least bad option...
	bool parse(Logger& logger);
	
	const std::string& getRawRequest() const
	{
		return m_rawRequest;
	}

	const std::string& getPath() const
	{
		return m_path;
	}
	
	HTTPRequestType getRequestType() const
	{
		return m_requestType;
	}

	const std::string& getHost() const
	{
		return m_hostValue;
	}

	HTTPVersion getHTTPVersion() const
	{
		return m_httpVersion;
	}

	ConnectionType getConnectionType() const
	{
		return m_connectionType;
	}

	FileType getFileType() const
	{
		return m_fileType;
	}

	bool hasAuthenticationHeader() const
	{
		return m_headerAuthenticationType != eAuthNone;
	}

	bool isAcceptedAuthenticationHeader() const
	{
		return m_headerAuthenticationType == eAuthBasic;
	}

	const std::string& getAuthUsername() const
	{
		return m_authUsername;
	}

	const std::string& getAuthPassword() const
	{
		return m_authPassword;
	}
	
	const std::string& getUserAgent() const
	{
		return m_userAgentField;
	}

	bool hasParams() const
	{
		return !m_aParams.empty();
	}

	bool hasParam(const std::string& name) const;
	std::string getParam(const std::string& name) const;
	int getParamAsInt(const std::string& name, int defaultVal) const;
	
	std::string getParamsAsGETString(bool ignorePaginationParams) const;

	bool hasCookies() const
	{
		return !m_aCookies.empty();
	}

	bool hasCookie(const std::string& name) const;
	std::string getCookie(const std::string& name) const;
	int getCookieAsInt(const std::string& name, int defaultVal = -1) const;

	// looks for a param (first) or a cookie (second) and returns value
	int getParamOrCookieAsInt(const std::string& paramName, const std::string& cookieName, int defaultValue) const;

protected:
	static std::string extractFieldItem(const std::string& fieldString, unsigned int valueStartPos);
	void addParams(const std::string& params);

	void processAuthenticationHeader(const std::string& authorizationString);
	void processCookieHeader(const std::string& cookieString);

protected:
	std::string				m_rawRequest;

	// extracted field strings
	std::string				m_userAgentField;

	// decoded / processed items
	HTTPRequestType			m_requestType;
	HTTPVersion				m_httpVersion;

	std::string				m_path;

	std::string				m_hostValue;
	std::string				m_connectionValue;

	// Note: this might be set based on HTTP version default and modified by Connection header field...
	ConnectionType			m_connectionType;

	FileType				m_fileType;

	AuthenticationType		m_headerAuthenticationType;
	std::string				m_authUsername;
	std::string				m_authPassword;

	std::map<std::string, std::string>	m_aParams;
	std::map<std::string, std::string>	m_aCookies;
};

#endif // WEB_REQUEST_H
