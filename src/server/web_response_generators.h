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

#ifndef WEB_RESPONSE_GENERATORS_H
#define WEB_RESPONSE_GENERATORS_H

#include <string>
#include <vector>

#include "web_response.h"

class WebResponseGenerator
{
public:
	WebResponseGenerator()
	{

	}

	virtual std::string getResponseString(const WebResponseParams& responseParams) const = 0;
};

class WebResponseGeneratorBasicText : public WebResponseGenerator
{
public:
	WebResponseGeneratorBasicText(int returnCode, const std::string& text);

	virtual std::string getResponseString(const WebResponseParams& responseParams) const override;

protected:
	int				m_returnCode;
	std::string		m_text;
};

class WebResponseGeneratorRedirect : public WebResponseGenerator
{
public:
	WebResponseGeneratorRedirect(const std::string& redirectURL, int redirectStatusCode = 303);

	virtual std::string getResponseString(const WebResponseParams& responseParams) const override;

protected:
	std::string		m_redirectUrl;
	int				m_statusCode;
};

class WebResponseGeneratorRedirectSetCookie : public WebResponseGenerator
{
public:
	WebResponseGeneratorRedirectSetCookie(const std::string& redirectURL, const std::string& cookieName,
										  const std::string& cookieValue);

	void setCookieDomain(const std::string& cookieDomain)
	{
		m_cookieDomain = cookieDomain;
	}

	void setCookiePath(const std::string& cookiePath)
	{
		m_cookiePath = cookiePath;
	}

	void setCookieMaxAge(unsigned int cookieMaxAgeInMinutes)
	{
		m_cookieMaxAgeInMinutes = cookieMaxAgeInMinutes;
	}

	void setCookieHttpOnly(bool cookieHttpOnly)
	{
		m_cookieHttpOnly = cookieHttpOnly;
	}

	virtual std::string getResponseString(const WebResponseParams& responseParams) const override;

protected:
	std::string		m_redirectUrl;
	std::string		m_cookieName;
	std::string		m_cookieValue;
	std::string		m_cookieDomain;
	std::string		m_cookiePath;

	unsigned int	m_cookieMaxAgeInMinutes;
	bool			m_cookieHttpOnly;
};

class WebResponseGeneratorAuthentication : public WebResponseGenerator
{
public:
	WebResponseGeneratorAuthentication();

	virtual std::string getResponseString(const WebResponseParams& responseParams) const override;

protected:

};

class WebResponseGeneratorFile : public WebResponseGenerator
{
public:
	WebResponseGeneratorFile(const std::string& path);

	virtual std::string getResponseString(const WebResponseParams& responseParams) const override;

	enum FileContentType
	{
		eContentTextHTML,
		eContentTextCSS,
		eContentTextJS,
		eContentImagePNG,
		eContentImageJPEG,
		eContentImageSVG,
		eContentUnknown
	};

protected:
	std::string		m_path;
};

class WebResponseGeneratorTemplateFile : public WebResponseGenerator
{
public:
	WebResponseGeneratorTemplateFile(const std::string& path, const std::string& content);
	WebResponseGeneratorTemplateFile(const std::string& path, const std::string& content1, const std::string& content2);
	WebResponseGeneratorTemplateFile(const std::string& path, const std::string& content1, const std::string& content2, const std::string& content3);
	WebResponseGeneratorTemplateFile(const std::string& path, const std::string& content1, const std::string& content2, const std::string& content3, const std::string& content4);

	// TODO: if the above gets any more silly, use a function to add more instead!!

	virtual std::string getResponseString(const WebResponseParams& responseParams) const override;

protected:
	int							m_templateArgs;
	std::string					m_path;
	std::vector<std::string>	m_aContent;
};

#endif // WEB_RESPONSE_GENERATORS_H
