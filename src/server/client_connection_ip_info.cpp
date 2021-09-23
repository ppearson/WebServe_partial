/*
 WebServe
 Copyright 2019 Peter Pearson.

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
	m_ip(rhs.m_ip)
{

}

ClientConnectionIPInfo& ClientConnectionIPInfo::operator=(const ClientConnectionIPInfo& rhs)
{
	m_ipVersion = rhs.m_ipVersion;
	m_ip = rhs.m_ip;
	return *this;
}

bool ClientConnectionIPInfo::operator==(const ClientConnectionIPInfo& rhs) const
{
	if (m_ipVersion != rhs.m_ipVersion)
		return false;

	if (m_ipVersion == 4)
	{
		return m_ip == rhs.m_ip;
	}

	return false;
}

bool ClientConnectionIPInfo::initInfo(const Socket* pSocket)
{
	if (pSocket->m_addr.sin_family == AF_INET)
	{
		m_ipVersion = 4;
		m_ip = pSocket->m_addr.sin_addr.s_addr;
	}
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
		sockAddr.s_addr = m_ip;

		finalAddress = inet_ntoa(sockAddr);
	}

	return finalAddress;
}

HashValue ClientConnectionIPInfo::getHash() const
{
	Hash hash;
	hash.addUInt(m_ipVersion);

	if (m_ipVersion == 4)
	{
		hash.addUInt(m_ip);
	}

	return hash.getHash();
}
