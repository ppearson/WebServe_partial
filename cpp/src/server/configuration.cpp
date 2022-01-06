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

#include "configuration.h"

#include "utils/string_helpers.h"

#include <fstream>

Configuration::Configuration() :
	m_workerThreads(16),
	m_enableHTTPv4(true),
	m_portNumberHTTPv4(9393),
	m_enableHTTPSv4(false),
	m_portNumberHTTPSv4(9394),
	m_redirectToHTTPS(false),
	m_enableHTTPv6(false),
	m_portNumberHTTPv6(9393),
	m_enableHTTPSv6(false),
	m_portNumberHTTPSv6(9394),
	m_enableHSTS(false),
	m_logOutputEnabled(true),
	m_logOutputTarget("stderr"),
	m_logOutputLevel("warning"),
	m_downgradeUserAfterBind(false),
	m_accessControlEnabled(false),
	m_closeBanClientsEnabled(false),
	m_closeBanClientsFailsThreshold(5),
	m_closeBanClientsTime(20),
	m_404NotFoundResponsesEnabled(true),
	m_keepAliveEnabled(true),
	m_keepAliveTimeout(6),
	m_keepAliveLimit(20),
	m_chunkedTransferJPEGsEnabled(false),
	m_sendDateHeaderField(true),
	m_tcpFastOpen(false)
{

}

bool Configuration::autoLoadFile()
{
#ifdef __linux__
	return loadFromFile("/home/peter/webserve.ini");
#else
	return loadFromFile("/Users/peter/webserve.ini");
#endif
}

bool Configuration::loadFromFile(const std::string& configPath)
{
	std::fstream fileStream(configPath.c_str(), std::ios::in);
	if (fileStream.fail())
	{
		return false;
	}

	std::string line;
	char buf[2048];

	std::string key;
	std::string value;
	
	SiteConfig* pCurrentSiteConfig = nullptr;

	while (fileStream.getline(buf, 2048))
	{
		line.assign(buf);

		// ignore empty lines and comments
		if (buf[0] == 0 || buf[0] == '#')
			continue;
		
		// first of all, see if it's a site definition statement
		if (line.find(" =") != std::string::npos && line.find("site(") != std::string::npos)
		{
			// it's a site config definition statement, i.e.:
			// <name> = site(<type>, <config>)
			// "photos = site(photos, host:photos.mydomain.net)
			
			m_aSiteConfigs.push_back(SiteConfig());
			
			pCurrentSiteConfig = &m_aSiteConfigs.back();
			
			size_t namePos = line.find(' ');
			std::string name = line.substr(0, namePos);
			
			pCurrentSiteConfig->m_name = name;
			
			size_t siteDefStart = line.find("site(");
			size_t siteDefEnd = line.find(')');
			
			if (siteDefStart != std::string::npos && siteDefEnd != std::string::npos &&
				siteDefStart < siteDefEnd)
			{
				siteDefStart += 5;
				
				std::string siteDefContents = line.substr(siteDefStart, siteDefEnd - siteDefStart);
				
				std::string siteDefType;
				std::string siteDefConfig;
				
				StringHelpers::splitInTwo(siteDefContents, siteDefType, siteDefConfig, ",");
				
				StringHelpers::stripWhitespace(siteDefType);
				StringHelpers::stripWhitespace(siteDefConfig);
				
				pCurrentSiteConfig->m_type = siteDefType;
				pCurrentSiteConfig->m_definition = siteDefConfig;
			}
			
			continue;
		}
		
		// see if we're setting a param for a site, with a '.' in the initial name, i.e.
		// photos.webContentPath: 1
		
		if (line.find(": ") != std::string::npos)
		{
			size_t colonPos = line.find(':');
			size_t dotPos = line.find('.');
			
			if (colonPos != std::string::npos && dotPos != std::string::npos &&
				dotPos < colonPos)
			{
				std::string fullName = line.substr(0, colonPos);
				
				// site it belongs to - for the moment, we'll ignore this and use the last, but
				// will need to fix this up in the future.
				std::string siteName = fullName.substr(0, dotPos);
				
				std::string paramName = fullName.substr(dotPos + 1);
				
				std::string stringValue = line.substr(colonPos + 1);
				StringHelpers::stripWhitespace(stringValue);
				
				pCurrentSiteConfig->m_aParams[paramName] = stringValue;
				
				continue;
			}
		}		

		if (!getKeyValue(line, key, value))
		{
			// TODO: error output
			continue;
		}

		// TODO: create some function infrastructure to handle this whole series of statements more concisely...
		if (key == "workerThreads")
		{
			unsigned int intValue = atoi(value.c_str());
			m_workerThreads = intValue;
		}
		else if (tryExtractBoolValue("enableHTTP", key, value, m_enableHTTPv4) || tryExtractBoolValue("enableHTTPv4", key, value, m_enableHTTPv4))
		{
			
		}
		else if (key == "portNumberHTTP" || key == "portNumberHTTPv4")
		{
			unsigned int intValue = atoi(value.c_str());
			m_portNumberHTTPv4 = intValue;
		}
		else if (tryExtractBoolValue("enableHTTPS", key, value, m_enableHTTPSv4) || tryExtractBoolValue("enableHTTPSv4", key, value, m_enableHTTPSv4))
		{
			
		}
		else if (key == "portNumberHTTPS" || key == "portNumberHTTPSv4")
		{
			unsigned int intValue = atoi(value.c_str());
			m_portNumberHTTPSv4 = intValue;
		}
		else if (tryExtractBoolValue("enableHTTPv6", key, value, m_enableHTTPv6))
		{

		}
		else if (key == "portNumberHTTPv6")
		{
			unsigned int intValue = atoi(value.c_str());
			m_portNumberHTTPv6 = intValue;
		}
		else if (tryExtractBoolValue("enableHTTPSv6", key, value, m_enableHTTPSv6))
		{

		}
		else if (key == "portNumberHTTPSv6")
		{
			unsigned int intValue = atoi(value.c_str());
			m_portNumberHTTPSv6 = intValue;
		}
		else if (key == "httpsCertificatePath")
		{
			// these https cert/key path items are a bit different, in that we allow multiple,
			// so we just emplace_back() them into the vector, rather than setting them directly.
			// TODO: the semantics within the .ini file for this are a bit odd, so maybe we
			//       use += syntax instead of the default?
			if (!value.empty())
			{
				m_httpsCertificatePaths.emplace_back(value);
			}
		}
		else if (key == "httpsKeyPath")
		{
			// these https cert/key path items are a bit different, in that we allow multiple,
			// so we just emplace_back() them into the vector, rather than setting them directly.
			// TODO: the semantics within the .ini file for this are a bit odd, so maybe we
			//       use += syntax instead of the default?
			if (!value.empty())
			{
				m_httpsKeyPaths.emplace_back(value);
			}
		}
		else if (tryExtractBoolValue("redirectToHTTPS", key, value, m_redirectToHTTPS))
		{
			
		}
		else if (tryExtractBoolValue("enableHSTS", key, value, m_enableHSTS))
		{
			
		}
		else if (tryExtractBoolValue("logOutputEnabled", key, value, m_logOutputEnabled))
		{

		}
		else if (key == "logOutputLevel")
		{
			if (!value.empty())
			{
				m_logOutputLevel = value;
			}
			else
			{
				// error, and/or turn off downgradeUserAfterBind ?
			}
		}
		else if (key == "logOutputTarget")
		{
			if (!value.empty())
			{
				m_logOutputTarget = value;
			}
		}
		else if (tryExtractBoolValue("downgradeUserAfterBind", key, value, m_downgradeUserAfterBind))
		{

		}
		else if (key == "downgradeUserName")
		{
			if (!value.empty())
			{
				m_downgradeUserName = value;
			}
			else
			{
				// error, and/or turn off downgradeUserAfterBind ?
			}
		}
		else if (tryExtractBoolValue("accessControlEnabled", key, value, m_accessControlEnabled))
		{

		}
		else if (key == "accessControlLogTarget")
		{
			if (!value.empty())
			{
				m_accessControlLogTarget = value;
			}
		}
		else if (tryExtractBoolValue("closeBanClientsEnabled", key, value, m_closeBanClientsEnabled))
		{

		}
		else if (key == "closeBanClientsFailsThreshold")
		{
			unsigned int intValue = atoi(value.c_str());
			m_closeBanClientsFailsThreshold = intValue;
		}
		else if (key == "closeBanClientsTime")
		{
			unsigned int intValue = atoi(value.c_str());
			m_closeBanClientsTime = intValue;
		}
		else if (tryExtractBoolValue("404NotFoundResponsesEnabled", key, value, m_404NotFoundResponsesEnabled))
		{

		}
		else if (tryExtractBoolValue("keepAliveEnabled", key, value, m_keepAliveEnabled))
		{

		}
		else if (key == "keepAliveTimeout")
		{
			unsigned int intValue = atoi(value.c_str());
			m_keepAliveTimeout = intValue;
		}
		else if (key == "keepAliveLimit")
		{
			unsigned int intValue = atoi(value.c_str());
			m_keepAliveLimit = intValue;
		}
		else if (tryExtractBoolValue("sendDateHeaderField", key, value, m_sendDateHeaderField))
		{

		}
		else if (tryExtractBoolValue("chunkedTransferJPEGsEnabled", key, value, m_chunkedTransferJPEGsEnabled))
		{

		}
		else if (tryExtractBoolValue("tcpFastOpen", key, value, m_tcpFastOpen))
		{

		}
		else
		{
			// TODO: proper logging, but we currently don't have a logger at this point, so...
			fprintf(stderr, "Warning: Unrecognised configuration item: '%s' in WebServe config file...\n", key.c_str());
		}
	}

	fileStream.close();

	return true;
}

bool Configuration::tryExtractBoolValue(const std::string& tryKeyName, const std::string& actualKeyName,
										const std::string& keyValue, bool& configMemberVariable)
{
	if (tryKeyName != actualKeyName)
		return false;

	bool extractedBoolValue = false;
	if (getBoolValueFromString(keyValue, extractedBoolValue))
	{
		configMemberVariable = extractedBoolValue;
	}
	else
	{
		// TODO: error...
	}

	// return that the key name actually matched, even if we didn't actually successfully apply the config value...
	return true;
}

bool Configuration::getKeyValue(const std::string& configLine, std::string& key, std::string& value)
{
	size_t sepPos = configLine.find(':');
	if (sepPos == std::string::npos)
		return false;

	size_t valueStart = configLine.find_first_not_of(' ', sepPos + 1);
	if (valueStart == std::string::npos)
		return false;

	key = configLine.substr(0, sepPos);
	value = configLine.substr(valueStart);

	return true;
}

// note: this returns whether it extracted a value or not, not the actual value!!!
bool Configuration::getBoolValueFromString(const std::string& stringValue, bool& boolValue)
{
	if (stringValue.empty())
		return false;

	// check known strings first
	if (stringValue == "true" || stringValue == "yes")
	{
		boolValue = true;
		return true;
	}
	else if (stringValue == "false" || stringValue == "no")
	{
		boolValue = false;
		return true;
	}

	// otherwise attempt to see if it's a number value, 0 or 1...
	// TODO: obviously atoi() might return 0 on an invalid string, so this isn't really robust...
	unsigned int intValue = atoi(stringValue.c_str());
	if (intValue == 0)
	{
		boolValue = false;
		// TODO: this isn't really robust, so somewhat makes a mockery of doing it this way...
		return true;
	}
	else if (intValue == 1)
	{
		boolValue = true;
		return true;
	}

	return false;
}
