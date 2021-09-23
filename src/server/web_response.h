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

#ifndef WEB_RESPONSE_H
#define WEB_RESPONSE_H

#include <string>

class WebResponseAdvanced;

class Configuration;

struct WebResponseParams
{
	WebResponseParams(const Configuration& conf, bool secureConnection) :
		configuration(conf),
		keepAliveEnabled(true),
		useChunkedLargeFiles(false),
		cacheControlFlags(0),
		cacheControlMaxAgeValue(0),
		sendHSTSHeader(false)
	{
		extractParamsFromConfiguration(secureConnection);
	}

	enum CacheControlFlags
	{
		CC_PUBLIC				= 1 << 0,
		CC_PRIVATE				= 1 << 1,

		CC_MAX_AGE				= 1 << 2,

		CC_MUST_REVALIDATE		= 1 << 3,
		CC_NO_CACHE				= 1 << 4,
		CC_NO_STORE				= 1 << 5
	};

	void extractParamsFromConfiguration(bool secureConnection);

	void setCacheControlParams(unsigned int ccFlags, unsigned int maxAgeMinutes = 0)
	{
		cacheControlFlags = ccFlags;
		if (ccFlags & CC_MAX_AGE)
		{
			cacheControlMaxAgeValue = maxAgeMinutes * 60;
		}
	}

	//

	const Configuration&	configuration;

	bool					keepAliveEnabled;

	bool					useChunkedLargeFiles;

	unsigned int			cacheControlFlags;
	unsigned int			cacheControlMaxAgeValue;
	
	bool					sendHSTSHeader;
};

class WebResponseCommon
{
public:
	WebResponseCommon()
	{

	}

	static void addCommonResponseHeaderItems(std::string& headerResponse, const WebResponseParams& responseParams);
};

#endif // WEB_RESPONSE_H
