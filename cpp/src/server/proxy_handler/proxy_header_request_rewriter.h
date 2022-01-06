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

#ifndef PROXY_HEADER_REQUEST_REWRITER_H
#define PROXY_HEADER_REQUEST_REWRITER_H

#include <string>

class WebRequest;

class ProxyHeaderRequestRewriter
{
public:
	ProxyHeaderRequestRewriter();
	~ProxyHeaderRequestRewriter();
	
	void initialise(const std::string& targetHostname, int targetPort, const std::string& targetPath);
	
	std::string generateRewrittenProxyHeaderRequest(const WebRequest& originalRequest, const std::string& refinedURI) const;
	
	
protected:
	std::string		m_targetHostname;
	int				m_targetPort;
	
	std::string		m_targetPath;
};

#endif // PROXY_HEADER_REQUEST_REWRITER_H
