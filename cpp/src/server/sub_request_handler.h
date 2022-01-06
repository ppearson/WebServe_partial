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

#ifndef SUB_REQUEST_HANDLER_H
#define SUB_REQUEST_HANDLER_H

#include <string>

#include "request_handler_common.h"
#include "configuration.h"

struct RequestConnection;
class WebRequest;

class Logger;

class SubRequestHandler
{
public:
	SubRequestHandler()
	{
	}

	virtual ~SubRequestHandler()
	{
	}

	virtual void configure(const Configuration::SiteConfig& siteConfig, const Configuration& mainConfig, Logger& logger)
	{
	}

	virtual WebRequestHandlerResult handleRequest(RequestConnection& requestConnection, const WebRequest& request, const std::string& refinedURI) = 0;

protected:
};

#endif // SUB_REQUEST_HANDLER_H
