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

#ifndef CLIENT_CONNECTION_IP_INFO_H
#define CLIENT_CONNECTION_IP_INFO_H

#include <string>

#include <netinet/in.h>

#include "utils/hash.h"

class Socket;

class ClientConnectionIPInfo
{
public:
	ClientConnectionIPInfo();
	ClientConnectionIPInfo(const ClientConnectionIPInfo& rhs);
	
	ClientConnectionIPInfo& operator=(const ClientConnectionIPInfo& rhs);

	bool operator==(const ClientConnectionIPInfo& rhs) const;

	bool initInfo(const Socket* pSocket);

	std::string getIPAddress() const;

	HashValue getHash() const;

protected:
	unsigned int		m_ipVersion;

	in_addr_t			m_ipv4;
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	in6_addr			m_ipv6;
#endif
};

#endif // CLIENT_CONNECTION_IP_INFO_H
