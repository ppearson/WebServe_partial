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

#include "web_response.h"

#include <ctime>

#include "configuration.h"

void WebResponseParams::extractParamsFromConfiguration(bool secureConnection)
{
	keepAliveEnabled = configuration.getKeepAliveEnabled();
	// don't enable this for the moment...
//	useChunkedLargeFiles = configuration.getChunkedTransferJPEGsEnabled();
	
	sendHSTSHeader = configuration.isHSTSEnabled() && secureConnection;
}

void WebResponseCommon::addCommonResponseHeaderItems(std::string& headerResponse, const WebResponseParams& responseParams)
{
	if (responseParams.configuration.getSendDateHeaderField())
	{
		char szTime[64];

		time_t timeNow;
		struct tm timeInfo;
		time(&timeNow);
		gmtime_r(&timeNow, &timeInfo);
		strftime (szTime, 64, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", &timeInfo);
		headerResponse += szTime;
	}

	if (responseParams.keepAliveEnabled)
	{
//		headerResponse += "Connection: keep-alive\r\nKeep-Alive: timeout=5\r\n";
		headerResponse += "Connection: keep-alive\r\n";
	}
	else
	{
		headerResponse += "Connection: close\r\n";
	}

	if (responseParams.cacheControlFlags)
	{
		std::string cacheControlValue;

		// TODO: complete the implementation of this...

		if (responseParams.cacheControlFlags & WebResponseParams::CC_PRIVATE)
		{
			if (responseParams.cacheControlFlags & WebResponseParams::CC_NO_CACHE)
			{
				cacheControlValue = "private, no-cache";
			}
			else
			{
				cacheControlValue = "private";
			}
		}
		else if (responseParams.cacheControlFlags & WebResponseParams::CC_PUBLIC)
		{
			cacheControlValue = "public";
		}

		if (responseParams.cacheControlFlags & WebResponseParams::CC_MAX_AGE)
		{
			char szTemp[64];
			if (cacheControlValue.empty())
			{
				sprintf(szTemp, "max-age=%u", responseParams.cacheControlMaxAgeValue);
				cacheControlValue = szTemp;
			}
			else
			{
				sprintf(szTemp, ", max-age=%u", responseParams.cacheControlMaxAgeValue);
				cacheControlValue += szTemp;
			}
		}

		if (!cacheControlValue.empty())
		{
			headerResponse += "Cache-Control: " + cacheControlValue + "\r\n";
		}
	}
	
	// 30 days for the moment
	// we could send it always, as browsers should ignore it if it's not a secure connection, but...
	if (responseParams.sendHSTSHeader)
	{
		headerResponse += "Strict-Transport-Security: max-age=2592000; includeSubDomains\r\n";
	}
	
//	headerResponse += "Server: WebServe\r\n";
}
