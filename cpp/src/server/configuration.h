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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <map>
#include <vector>

class Configuration
{
public:
	Configuration();
	
	class SiteConfig
	{
	public:
		SiteConfig()
		{	
		}
		
		bool hasParam(const std::string& paramName) const
		{
			return m_aParams.count(paramName) > 0;
		}
		
		std::string getParam(const std::string& paramName) const
		{
			std::string retVal;
			
			std::map<std::string, std::string>::const_iterator itFind = m_aParams.find(paramName);
			if (itFind != m_aParams.end())
			{
				retVal = itFind->second;
			}
			
			return retVal;
		}
		
		bool getParamAsBool(const std::string& paramName, bool defaultVal) const
		{
			std::map<std::string, std::string>::const_iterator itFind = m_aParams.find(paramName);
			if (itFind != m_aParams.end())
			{
				std::string strVal = itFind->second;
				if (strVal == "1" || strVal == "true" || strVal == "yes")
					return true;
				else
					return false;
			}
			else
			{
				return defaultVal;
			}
		}
		
		unsigned int getParamAsUInt(const std::string& paramName, unsigned int defaultVal) const
		{
			std::map<std::string, std::string>::const_iterator itFind = m_aParams.find(paramName);
			if (itFind != m_aParams.end())
			{
				std::string strVal = itFind->second;
				unsigned int intVal = atoi(strVal.c_str());
				return intVal;
			}
			else
			{
				return defaultVal;
			}
		}
		
		std::string			m_name;
		std::string			m_type;
		
		std::string			m_definition;
		
		std::map<std::string, std::string>	m_aParams;
	};

	bool autoLoadFile();

	bool loadFromFile(const std::string& configPath);

	// TODO: key/value lookup for this stuff...

	unsigned int getNumWorkerThreads() const
	{
		return m_workerThreads;
	}
	
	bool isHTTPv4Enabled() const
	{
		return m_enableHTTPv4;
	}

	unsigned int getHTTPv4PortNumber() const
	{
		return m_portNumberHTTPv4;
	}
	
	bool isHTTPSv4Enabled() const
	{
		return m_enableHTTPSv4;
	}
	
	unsigned int getHTTPSv4PortNumber() const
	{
		return m_portNumberHTTPSv4;
	}

	bool isHTTPv6Enabled() const
	{
		return m_enableHTTPv6;
	}

	unsigned int getHTTPv6PortNumber() const
	{
		return m_portNumberHTTPv6;
	}
	
	bool isHTTPSv6Enabled() const
	{
		return m_enableHTTPSv6;
	}
	
	unsigned int getHTTPSv6PortNumber() const
	{
		return m_portNumberHTTPSv6;
	}
	
	const std::vector<std::string>& getHTTPSCertificatePaths() const
	{
		return m_httpsCertificatePaths;
	}
	
	const std::vector<std::string>& getHTTPSKeyPaths() const
	{
		return m_httpsKeyPaths;
	}
	
	bool isRedirectToHTTPSEnabled() const
	{
		return m_redirectToHTTPS;
	}
	
	bool isHSTSEnabled() const
	{
		return m_enableHSTS;
	}

	bool getLogOutputEnabled() const
	{
		return m_logOutputEnabled;
	}

	const std::string& getLogOutputTarget() const
	{
		return m_logOutputTarget;
	}

	const std::string& getLogOutputLevel() const
	{
		return m_logOutputLevel;
	}

	bool getDowngradeUserAfterBind() const
	{
		return m_downgradeUserAfterBind;
	}

	const std::string& getDowngradeUserName() const
	{
		return m_downgradeUserName;
	}

	bool getAccessControlEnabled() const
	{
		return m_accessControlEnabled;
	}

	const std::string& getAccessControlLogTarget() const
	{
		return m_accessControlLogTarget;
	}

	bool getCloseBanClientsEnabled() const
	{
		return m_closeBanClientsEnabled;
	}

	unsigned int getCloseBanClientsFailsThreshold() const
	{
		return m_closeBanClientsFailsThreshold;
	}

	unsigned int getCloseBanClientsTime() const
	{
		return m_closeBanClientsTime;
	}

	bool get404NotFoundResponsesEnabled() const
	{
		return m_404NotFoundResponsesEnabled;
	}

	bool getKeepAliveEnabled() const
	{
		return m_keepAliveEnabled;
	}

	unsigned int getKeepAliveTimeout() const
	{
		return m_keepAliveTimeout;
	}

	unsigned int getKeepAliveLimit() const
	{
		return m_keepAliveLimit;
	}

	bool getSendDateHeaderField() const
	{
		return m_sendDateHeaderField;
	}

	bool getChunkedTransferJPEGsEnabled() const
	{
		return m_chunkedTransferJPEGsEnabled;
	}

	bool getTcpFastOpen() const
	{
		return m_tcpFastOpen;
	}
	
	const std::vector<SiteConfig>& getSiteConfigs() const
	{
		return m_aSiteConfigs;
	}

protected:

	static bool tryExtractBoolValue(const std::string& tryKeyName, const std::string& actualKeyName,
									const std::string& keyValue, bool& configMemberVariable);

	static bool getKeyValue(const std::string& configLine, std::string& key, std::string& value);

	// note: this returns whether it extracted a value or not, not the actual value!!!
	static bool getBoolValueFromString(const std::string& stringValue, bool& boolValue);

protected:
	// webserve stuff
	unsigned int			m_workerThreads;
	
	// TODO: something less duplicate than this, and arguably more flexible as well,
	//       but this is at least something to work off functionally...
	bool					m_enableHTTPv4;
	unsigned int			m_portNumberHTTPv4;
	bool					m_enableHTTPSv4;
	unsigned int			m_portNumberHTTPSv4;

	bool					m_enableHTTPv6;
	unsigned int			m_portNumberHTTPv6;
	bool					m_enableHTTPSv6;
	unsigned int			m_portNumberHTTPSv6;
	
	// for HTTPS. A vector so that we can cope with multiple for different domains / virtual hosts.
	std::vector<std::string>	m_httpsCertificatePaths;
	std::vector<std::string>	m_httpsKeyPaths;
	
	bool					m_redirectToHTTPS;
	bool					m_enableHSTS; // send HSTS headers to ensure HTTPS only is used

	bool					m_logOutputEnabled;
	std::string				m_logOutputTarget;
	std::string				m_logOutputLevel;

	bool					m_downgradeUserAfterBind;
	std::string				m_downgradeUserName;

	//
	bool					m_accessControlEnabled;
	std::string				m_accessControlLogTarget;
	bool					m_closeBanClientsEnabled;
	unsigned int			m_closeBanClientsFailsThreshold;
	unsigned int			m_closeBanClientsTime;

	// Note: it's recommended this be enabled if access control close banning is enabled.
	bool					m_404NotFoundResponsesEnabled;

	// HTTP stuff
	bool					m_keepAliveEnabled;
	unsigned int			m_keepAliveTimeout;
	unsigned int			m_keepAliveLimit;

	bool					m_chunkedTransferJPEGsEnabled;

	// these things are slightly silly in that they're heavily recommended by the RFC, but it's useful for benchmarking
	bool					m_sendDateHeaderField;

	// TCP stuff
	bool					m_tcpFastOpen;
	
	std::vector<SiteConfig> m_aSiteConfigs;
};

#endif // CONFIGURATION_H
