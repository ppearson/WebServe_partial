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

#include "proxy_header_request_rewriter.h"

#include <vector>

#include "web_request.h"

#include "utils/string_helpers.h"

ProxyHeaderRequestRewriter::ProxyHeaderRequestRewriter()
{
	
}

ProxyHeaderRequestRewriter::~ProxyHeaderRequestRewriter()
{
	
}

void ProxyHeaderRequestRewriter::initialise(const std::string& targetHostname, int targetPort, const std::string& targetPath)
{
	m_targetHostname = targetHostname;
	m_targetPort = targetPort;
	m_targetPath = targetPath;
}

std::string ProxyHeaderRequestRewriter::generateRewrittenProxyHeaderRequest(const WebRequest& originalRequest, const std::string& refinedURI) const
{
	if (originalRequest.getRequestType() != WebRequest::eRequestGET &&
		originalRequest.getRequestType() != WebRequest::eRequestPOST)
	{
		return "";
	}
	
	std::string newHeaderRequest;
	
	std::vector<std::string> lines;
	StringHelpers::split(originalRequest.getRawRequest(), lines);
	
	// generate the first request line directly...
	newHeaderRequest += ((originalRequest.getRequestType() == WebRequest::eRequestGET) ? "GET" : "POST");
	newHeaderRequest += " ";
	
	std::string newProxyRequestPath = refinedURI;
	if (newProxyRequestPath.empty())
	{
		newProxyRequestPath = "/";
	}
	
	newHeaderRequest += newProxyRequestPath;
	
	newHeaderRequest += " HTTP/1.1\r\n";
	
	///
	
	std::vector<std::string>::const_iterator itLine = lines.begin();
	++ itLine; // skip the first line which we've handled already

	for (; itLine != lines.end(); ++itLine)
	{
		const std::string& otherLine = *itLine;
		
		if (otherLine.compare(0, 5, "Host:") == 0)
		{
			// replace the host with our target host
			newHeaderRequest += "Host: ";
			newHeaderRequest += m_targetHostname;
			newHeaderRequest += "\r\n";
		}
		else if (otherLine.compare(0, 16, "Accept-Encoding:") == 0)
		{
			// skip this for the minute, so that we don't ask the target for compressed content.
			continue;
		}
		else
		{
			newHeaderRequest += otherLine;
			// Note: we don't need '\r' here as it's already on the lines...
			newHeaderRequest += "\n";
		}
	}
	
	return newHeaderRequest;
}
