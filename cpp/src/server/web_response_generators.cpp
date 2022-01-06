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

#include "web_response_generators.h"

#include <fstream>
#include <sstream>

#include <cstring> // for memset()

#include "web_response.h"

#include "utils/string_helpers.h"

WebResponseGeneratorBasicText::WebResponseGeneratorBasicText(int returnCode, const std::string& text)
	: m_returnCode(returnCode),
	  m_text(text)
{

}

std::string WebResponseGeneratorBasicText::getResponseString(const WebResponseParams& responseParams) const
{
	std::string response;

	char szTemp[64];
	memset(szTemp, 0, 64);

	sprintf(szTemp, "HTTP/1.1 %i\r\n", m_returnCode);
	response += szTemp;

	WebResponseCommon::addCommonResponseHeaderItems(response, responseParams);

	response += "Content-Type: text/html; charset=UTF-8\r\n";

	memset(szTemp, 0, 64);
	sprintf(szTemp, "Content-Length: %ld\r\n\r\n", m_text.size());
	response += szTemp;

	response += m_text;

	return response;
}

//

WebResponseGeneratorRedirect::WebResponseGeneratorRedirect(const std::string& redirectURL, int redirectStatusCode) :
	m_redirectUrl(redirectURL),
	m_statusCode(redirectStatusCode)
{

}

std::string WebResponseGeneratorRedirect::getResponseString(const WebResponseParams& responseParams) const
{
	std::string response;

	char szTemp[2048];
	memset(szTemp, 0, 2048);

	sprintf(szTemp, "HTTP/1.1 %i\r\n", m_statusCode);
	response += szTemp;

	WebResponseCommon::addCommonResponseHeaderItems(response, responseParams);

	memset(szTemp, 0, 2048);
	snprintf(szTemp, 2048, "Location: %s\r\n", m_redirectUrl.c_str());
	response += szTemp;

	response += "Content-Length: 0\r\n\r\n";

	return response;
}

//

WebResponseGeneratorRedirectSetCookie::WebResponseGeneratorRedirectSetCookie(const std::string& redirectURL, const std::string& cookieName,
																			 const std::string& cookieValue) :
	m_redirectUrl(redirectURL),
	m_cookieName(cookieName),
	m_cookieValue(cookieValue),
	m_cookieMaxAgeInMinutes(0),
	m_cookieHttpOnly(false)
{

}

std::string WebResponseGeneratorRedirectSetCookie::getResponseString(const WebResponseParams& responseParams) const
{
	std::string response;

	char szTemp[256];
	memset(szTemp, 0, 256);

	sprintf(szTemp, "HTTP/1.1 %i\r\n", 303);
	response += szTemp;

	WebResponseCommon::addCommonResponseHeaderItems(response, responseParams);

	if (!m_cookieName.empty() && !m_cookieValue.empty())
	{
		// main statement...
		std::string setCookieStatement = "Set-Cookie: " + m_cookieName + "=" + m_cookieValue;

		// web browsers in theory should do this bit for us by default...
		if (!m_cookieDomain.empty())
		{
			setCookieStatement += "; Domain=" + m_cookieDomain;
		}

		if (!m_cookiePath.empty())
		{
			setCookieStatement += "; Path=" + m_cookiePath;
		}

		if (m_cookieMaxAgeInMinutes > 0)
		{
			setCookieStatement += "; Max-Age=" + std::to_string(m_cookieMaxAgeInMinutes * 60);
		}

		if (m_cookieHttpOnly)
		{
			setCookieStatement += "; HttpOnly";
		}

		response += setCookieStatement + "\r\n";
	}

	memset(szTemp, 0, 256);
	sprintf(szTemp, "Location: %s\r\n", m_redirectUrl.c_str());
	response += szTemp;

	response += "Content-Length: 0\r\n\r\n";

	return response;
}

//

WebResponseGeneratorAuthentication::WebResponseGeneratorAuthentication()
{

}

std::string WebResponseGeneratorAuthentication::getResponseString(const WebResponseParams& responseParams) const
{
	std::string response;

	char szTemp[128];
	memset(szTemp, 0, 128);

	sprintf(szTemp, "HTTP/1.1 %i Access Denied\r\n", 401);
	response += szTemp;

	//
	std::string authName = StringHelpers::generateRandomASCIIString(8) + "_";
	memset(szTemp, 0, 128);
	sprintf(szTemp, "WWW-Authenticate: Basic realm=\"%s\"\r\n", authName.c_str());
	response += szTemp;

	WebResponseCommon::addCommonResponseHeaderItems(response, responseParams);

	response += "Content-Length: 0\r\n\r\n";

	return response;
}

//

WebResponseGeneratorFile::WebResponseGeneratorFile(const std::string& path) :
	m_path(path)
{

}

// TODO: is having this class even necessary any more? What benefits does it provide over WebResponseAdvancedBinaryFile?

std::string WebResponseGeneratorFile::getResponseString(const WebResponseParams& responseParams) const
{
	std::string response;

	bool binary = false;
	FileContentType contentType = eContentTextHTML;
	// work out if it's an image
	int extensionPos = m_path.rfind('.');
	if (extensionPos != -1)
	{
		std::string extension = m_path.substr(extensionPos + 1);

		// Note: this is likely no longer a good idea, and everything's set to binary anyway
		//       due to CSS/JS minification causing extremely long lines, so...
		if (extension == "png")
		{
			binary = true;
			contentType = eContentImagePNG;
		}
		else if (extension == "jpg")
		{
			binary = true;
			contentType = eContentImageJPEG;
		}
		else if (extension == "svg")
		{
			binary = true;
			contentType = eContentImageSVG;
		}
		else if (extension == "css")
		{
			binary = true;
			contentType = eContentTextCSS;
		}
		else if (extension == "js")
		{
			binary = true;
			contentType = eContentTextJS;
		}
	}

	std::ios::openmode mode = std::ios::in;

	if (binary)
	{
		mode = std::ios::in | std::ios::binary;
	}

	std::fstream fileStream(m_path.c_str(), mode);

	std::string content;
	int returnCode = 200;

	if (fileStream.fail())
	{
		// TODO: for security, don't show the full path...
		content = "File not found.\n";
		returnCode = 404;
	}
	else
	{
		if (!binary)
		{
			std::string line;
			char buf[1024];

			// Note: this can be a serious limitation with minified JS/CSS...
			while (fileStream.getline(buf, 1024))
			{
				line.assign(buf);
				content += line + "\n";
			}
		}
		else
		{
			std::stringstream ssOut;
			ssOut << fileStream.rdbuf();

			content = ssOut.str();
		}
	}
	fileStream.close();

	char szTemp[64];
	memset(szTemp, 0, 64);

	sprintf(szTemp, "HTTP/1.1 %i\r\n", returnCode);
	response += szTemp;

	std::string contentTypeString;

	switch (contentType)
	{
		case eContentImagePNG:
			contentTypeString = "image/png";
			break;
		case eContentImageJPEG:
			contentTypeString = "image/jpeg";
			break;
		case eContentImageSVG:
			contentTypeString = "image/svg+xml";
			break;
		case eContentTextCSS:
			contentTypeString = "text/css; charset=UTF-8";
			break;
		case eContentTextJS:
			contentTypeString = "application/javascript";
			break;
		case eContentTextHTML:
			contentTypeString = "text/html; charset=UTF-8";
			break;
		default:
			break;
	}

	WebResponseCommon::addCommonResponseHeaderItems(response, responseParams);

	if (!contentTypeString.empty())
	{
		response += "Content-Type: " + contentTypeString + "\r\n";
	}

	memset(szTemp, 0, 64);
	sprintf(szTemp, "Content-Length: %zu\r\n\r\n", content.size());
	response += szTemp;

	response += content;

	return response;
}

//

WebResponseGeneratorTemplateFile::WebResponseGeneratorTemplateFile(const std::string& path, const std::string& content) : m_path(path)
{
	m_templateArgs = 1;

	m_aContent.push_back(content);
}

WebResponseGeneratorTemplateFile::WebResponseGeneratorTemplateFile(const std::string& path, const std::string& content1, const std::string& content2) : m_path(path)
{
	m_templateArgs = 2;

	m_aContent.push_back(content1);
	m_aContent.push_back(content2);
}

WebResponseGeneratorTemplateFile::WebResponseGeneratorTemplateFile(const std::string& path, const std::string& content1, const std::string& content2, const std::string& content3)
	: m_path(path)
{
	m_templateArgs = 3;

	m_aContent.push_back(content1);
	m_aContent.push_back(content2);
	m_aContent.push_back(content3);
}

WebResponseGeneratorTemplateFile::WebResponseGeneratorTemplateFile(const std::string& path, const std::string& content1, const std::string& content2,
																   const std::string& content3, const std::string& content4)
	: m_path(path)
{
	m_templateArgs = 4;

	m_aContent.push_back(content1);
	m_aContent.push_back(content2);
	m_aContent.push_back(content3);
	m_aContent.push_back(content4);
}

static const std::string templatePlaceholders[] = {"<%1%>", "<%2%>", "<%3%>", "<%4%>"};

std::string WebResponseGeneratorTemplateFile::getResponseString(const WebResponseParams& responseParams) const
{
	std::string response;

	std::fstream fileStream(m_path.c_str(), std::ios::in);

	std::string content;
	int returnCode = 200;

	if (fileStream.fail())
	{
		content = "Template file not found.\n";
		returnCode = 404;
	}
	else
	{
		if (m_templateArgs == 1)
		{
			bool doneReplace = false;
			std::string line;
			char buf[1024];

			while (fileStream.getline(buf, 1024))
			{
				line.assign(buf);

				// replace template with content if found

				if (!doneReplace) // only once per file - should make things slightly faster...
				{
					int nPlacement = line.find("<%%>");

					if (nPlacement != -1)
					{
						line.replace(nPlacement, 4, m_aContent[0]);
						doneReplace = true;
					}
				}

				content += line + "\n";
			}
		}
		else
		{
			std::string line;
			char buf[1024];

			int thisArg = 0;

			while (fileStream.getline(buf, 1024))
			{
				line.assign(buf);

				// replace template with content if found

				if (thisArg < m_templateArgs)
				{
					int nPlacement = line.find(templatePlaceholders[thisArg]);
					if (nPlacement != -1)
					{
						line.replace(nPlacement, 5, m_aContent[thisArg++]);
					}
				}

				content += line + "\n";
			}
		}
	}
	fileStream.close();

	char szTemp[64];
	memset(szTemp, 0, 64);

	sprintf(szTemp, "HTTP/1.1 %i\r\n", returnCode);
	response += szTemp;

	WebResponseCommon::addCommonResponseHeaderItems(response, responseParams);

	response += "Content-Type: text/html; charset=UTF-8\r\n";

	memset(szTemp, 0, 64);
	sprintf(szTemp, "Content-Length: %ld\r\n\r\n", content.size());
	response += szTemp;

	response += content;

	return response;
}
