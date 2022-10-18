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

#include "client_connection_ip_info.h"

#include "utils/socket.h"

ClientConnectionIPInfo::ClientConnectionIPInfo() :
	m_ipVersion(0)
{

}

ClientConnectionIPInfo::ClientConnectionIPInfo(const ClientConnectionIPInfo& rhs) :
	m_ipVersion(rhs.m_ipVersion),
	m_ipv4(rhs.m_ipv4)
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	,m_ipv6(rhs.m_ipv6)
#endif
{

}

ClientConnectionIPInfo& ClientConnectionIPInfo::operator=(const ClientConnectionIPInfo& rhs)
{
	m_ipVersion = rhs.m_ipVersion;
	m_ipv4 = rhs.m_ipv4;
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	m_ipv6 = rhs.m_ipv6;
#endif
	return *this;
}

bool ClientConnectionIPInfo::operator==(const ClientConnectionIPInfo& rhs) const
{
	if (m_ipVersion != rhs.m_ipVersion)
		return false;

	if (m_ipVersion == 4)
	{
		return m_ipv4 == rhs.m_ipv4;
	}
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	else if (m_ipVersion == 6)
	{
//		for (unsigned int i = 0; i < 4; i++)
//		{
//#if __APPLE__
//			if (m_ipv6.__u6_addr.__u6_addr32[i] != rhs.m_ipv6.__u6_addr.__u6_addr32[i])
//#else
//			if (m_ipv6.__in6_u.__u6_addr32[i] != rhs.m_ipv6.__in6_u.__u6_addr32[i])
//#endif
//			{
//				return false;
//			}
//		}

		return memcmp((const void*)&m_ipv6, (const void*)&rhs.m_ipv6, sizeof(m_ipv6));
	}
#endif

	return false;
}

bool ClientConnectionIPInfo::initInfo(const Socket* pSocket)
{
	if (pSocket->m_addr.ss_family == AF_INET)
	{
		m_ipVersion = 4;
		m_ipv4 = ((struct sockaddr_in*)&pSocket->m_addr)->sin_addr.s_addr;
	}
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	else if (pSocket->m_addr.ss_family == AF_INET6)
	{
		m_ipVersion = 6;
		m_ipv6 = ((struct sockaddr_in6*)&pSocket->m_addr)->sin6_addr;
	}
#endif
	else
	{
		return false;
	}

	return true;
}

std::string ClientConnectionIPInfo::getIPAddress() const
{
	std::string finalAddress;

	if (m_ipVersion == 4)
	{
		struct in_addr sockAddr;
		sockAddr.s_addr = m_ipv4;

		finalAddress = inet_ntoa(sockAddr);
	}
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	else if (m_ipVersion == 6)
	{
		struct in6_addr sockAddr;
		char szTemp[INET6_ADDRSTRLEN];
		if (inet_ntop(AF_INET6, &(m_ipv6), szTemp, INET6_ADDRSTRLEN) != nullptr)
		{
			finalAddress.append(szTemp);
		}
	}
#endif

	return finalAddress;
}

HashValue ClientConnectionIPInfo::getHash() const
{
	Hash hash;
	hash.addUInt(m_ipVersion);

	if (m_ipVersion == 4)
	{
		hash.addUInt(m_ipv4);
	}
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	else if (m_ipVersion == 6)
	{
//		for (unsigned int i = 0; i < 4; i++)
//		{
//#if __APPLE__
//			hash.addUInt(m_ipv6.__u6_addr.__u6_addr32[i]);
//#else
//			hash.addUInt(m_ipv6.__in6_u.__u6_addr32[i]);
//#endif
//		}
		hash.addData((const unsigned char*)&m_ipv6, sizeof(m_ipv6));
	}
#endif

	return hash.getHash();
}
