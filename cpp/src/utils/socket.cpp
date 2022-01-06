/*
 WebServe
 Copyright 2018-2022 Peter Pearson.
 taken originally from:
 Sitemon
 Copyright 2010 Peter Pearson.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
 */

#include "socket.h"

#include <cerrno>
#include <cstdlib>

#include <unistd.h>
#include <poll.h>

#include "string_helpers.h"

#define DISABLE_SIGPIPE_SOCKET 1

// unfortunately, there's no cross-platform way of doing this, so we need to specialise...
#if DISABLE_SIGPIPE_SOCKET
#ifdef __linux__
	#define USE_MSG_NOSIGNAL 1
#elif __APPLE__
	#define USE_NOSIGPIPE_SOCKOPT 1
#endif
#endif

#define USE_POLL_FOR_RECV_TIMEOUT 1

#define ENABLE_TCP_FASTOPEN_SUPPORT 0

#if ENABLE_TCP_FASTOPEN_SUPPORT
#ifdef __linux__
	#include <netinet/tcp.h>
#else
	#undef ENABLE_TCP_FASTOPEN_SUPPORT
#endif
#endif

static const unsigned int kMaxSendLength = 1024;
static const unsigned int kMaxRecvLength = 4096;

Socket::Socket() : m_version(0), m_sock(-1), m_port(-1), m_pLogger(nullptr)
#if SOCK_LEAK_DETECTOR
	, m_pLeak(nullptr)
#endif
{
	memset(&m_addr, 0, sizeof(m_addr));
}

Socket::Socket(Logger* pLogger, const std::string& host, int port, bool v6) : m_version(0), m_sock(-1), m_port(port), m_host(host), m_pLogger(nullptr)
#if SOCK_LEAK_DETECTOR
	, m_pLeak(nullptr)
#endif
{
	memset(&m_addr, 0, sizeof(m_addr));
	create(pLogger, 0, v6);
}

Socket::~Socket()
{
	close();
}

bool Socket::create(Logger* pLogger, unsigned int flags, bool v6)
{
	if (!v6)
	{
		m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		m_version = 4;
	}
	else
	{
#if WEBSERVE_ENABLE_IPV6_SUPPORT
		m_sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_IP);
		m_version = 6;
#else
		return false;
#endif
	}

	m_pLogger = pLogger;
	
	if (!isValid())
		return false;
	
#if SOCK_LEAK_DETECTOR
	m_pLeak = new char[64];
	memset(m_pLeak, 0, 64);
#endif
	
	int on = 1;
	if (::setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) == -1)
		return false;

#if USE_NOSIGPIPE_SOCKOPT
	if (::setsockopt(m_sock, SOL_SOCKET, SO_NOSIGPIPE, (const char*)&on, sizeof(on)) == -1)
		return false;
#endif
	
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	if (v6)
	{
		// Linux defaults this to off, but MacOS has it on by default, and until we refactor the socket
		// logic to nicely handle dual v4/v6 interfaces (which may or may not be what we want to do?), keep
		// them completely separate.
		if (::setsockopt(m_sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&on, sizeof(on)) == -1)
		{
			int errorNumber = errno;
			fprintf(stderr, "Socket setsockopt(..,IPV6_V6ONLY,..) error: %s\n", strerror(errorNumber));
			return false;
		}
	}
#endif

	if (flags & SOCKOPT_FASTOPEN)
	{
#if ENABLE_TCP_FASTOPEN_SUPPORT
		int qlen = 5;
		setsockopt(m_sock, SOL_TCP, TCP_FASTOPEN, &qlen, sizeof(qlen));
#else
		m_pLogger->warning("TCP fast open is not enabled in this build. Option will be ignored.");
#endif
	}
	
	return true;
}

bool Socket::bind(const int port, bool v6)
{
	if (!isValid())
		return false;

	socklen_t addrLen = 0;

	// TODO: do we need to memset() the struct fully?

	if (!v6)
	{
		sockaddr_in* v4 = (sockaddr_in*)&m_addr;
		v4->sin_family = AF_INET;
		v4->sin_addr.s_addr = htonl(INADDR_ANY);
		v4->sin_port = htons(port);

		addrLen = sizeof(sockaddr_in);

		m_version = 4;
	}
	else
	{
#if WEBSERVE_ENABLE_IPV6_SUPPORT
		sockaddr_in6* v6 = (sockaddr_in6*)&m_addr;
		v6->sin6_family = AF_INET6;
		v6->sin6_flowinfo = 0;

//		v6->sin6_scope_id = ?; // TODO....
		v6->sin6_port = htons(port);
		v6->sin6_addr = in6addr_any;

		addrLen = sizeof(sockaddr_in6);

		m_version = 6;
#else

#endif
	}
	
	int ret = ::bind(m_sock, (struct sockaddr*)&m_addr, addrLen);
	if (ret == -1)
	{
		int errorNumber = errno;
		fprintf(stderr, "Socket bind() error: %s\n", strerror(errorNumber));
		return false;
	}
	
	return true;	
}

bool Socket::listen(const int connections) const
{
	if (!isValid())
		return false;
	
	int ret = ::listen(m_sock, connections);
	if (ret == -1)
	{
		int errorNumber = errno;
		fprintf(stderr, "Socket listen() error: %s\n", strerror(errorNumber));
		return false;
	}
	
	return true;
}

bool Socket::connect()
{
	if (!isValid())
		return false;
	
	if (m_version == 4)
	{
		sockaddr_in* v4 = (sockaddr_in*)&m_addr;

		v4->sin_family = AF_INET;
		v4->sin_addr.s_addr = inet_addr(m_host.c_str());
		v4->sin_port = htons(m_port);

	//	status = inet_pton(AF_INET, m_host.c_str(), &m_addr.sin_addr);
		struct hostent *hp;
		hp = gethostbyname(m_host.c_str());
		v4->sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	}
	else
	{
		return false;
	}

	int status = ::connect(m_sock, (sockaddr*)&m_addr, sizeof(m_addr));
	
	if (status != 0)
		return false;
	
	return true;
}

bool Socket::connect(const std::string& host, const int port)
{
	m_host = host;
	m_port = port;
	
	return connect();
}

void Socket::close()
{
	if (isValid())
	{
		::shutdown(m_sock, SHUT_RDWR);
		::close(m_sock);
		
#if SOCK_LEAK_DETECTOR
		if (m_pLeak)
		{
			delete [] m_pLeak;
		}
#endif

		m_sock = -1;
	}
}

bool Socket::setRecvTimeoutOption(int timeoutSeconds)
{
	struct timeval timeout;
	timeout.tv_sec = timeoutSeconds;
	timeout.tv_usec = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
	{
		int errorNumber = errno;
		return false;
	}
	
	return true;
}

bool Socket::accept(Socket* sock) const
{
	if (!isValid())
		return false;
	
	socklen_t addrSize = sizeof(sock->m_addr);
	sock->m_sock = ::accept(m_sock, (sockaddr*)&sock->m_addr, &addrSize);
	
	if (sock->m_sock <= 0)
		return false;

	if (sock->m_addr.ss_family == AF_INET)
	{
		sock->m_version = 4;
	}
#if WEBSERVE_ENABLE_IPV6_SUPPORT
	else if (sock->m_addr.ss_family == AF_INET6)
	{
		sock->m_version = 6;
	}
#else
	return false;
#endif
	
#if SOCK_LEAK_DETECTOR
	sock->m_pLeak = new char[64];
	memset(sock->m_pLeak, 0, 64);
#endif
	
	return true;	
}

bool Socket::send(const std::string& data) const
{
	if (!isValid())
		return false;
	
	// TODO: use kMaxSendLength
#if USE_MSG_NOSIGNAL
	int bytesSent = ::send(m_sock, data.c_str(), data.size(), MSG_NOSIGNAL);
#else
	int bytesSent = ::send(m_sock, data.c_str(), data.size(), 0);
#endif
	
	if (bytesSent == -1)
		return false;
	
	return true;
}

bool Socket::send(unsigned char* pData, size_t dataLength) const
{
	if (!isValid())
		return false;

	// TODO: use kMaxSendLength
#if USE_MSG_NOSIGNAL
	int bytesSent = ::send(m_sock, pData, dataLength, MSG_NOSIGNAL);
#else
	int bytesSent = ::send(m_sock, pData, dataLength, 0);
#endif

	if (bytesSent == -1)
	{
		int errorNumber = errno;
		if (errorNumber == EPIPE) // 32
		{
			// client closed the connection
			return false;
		}
		return false;
	}
	else if (bytesSent != (int)dataLength)
	{
		// not sure what we can/should do if we ever get here (but we do, both Firefox and Safari often do this)...
		
		// in blocking mode, send() should block until it can send, so it's effectively
		// atomic in terms of either failing or sending all the data.
		
		// I assume the socket is actually getting closed by the client and because MSG_NOSIGNAL is set
		// this is all the notification we get?
		
		// Sending it again in all cases so far returns -1, which seems to support the above...
		
		// TODO: maybe we should properly buffer the above sending in chunks, and attempt to retry when
		//       this condition is hit, and then if that further attempt returns -1 abort?
		m_pLogger->info("send() returned unexpected incomplete value. Aborting transfer.");
		
		return false;
	}

	// TODO: check bytes sent was correct amount. Not sure how to recover from that if that isn't the case though...

	return true;
}

SocketRecvReturnCode Socket::recv(std::string& data) const
{
	if (!isValid())
		return eSockRecv_Error;
	
	int dataLength = 0;
	
	char buffer[kMaxRecvLength + 1];
		
	int ret = 0;
	
	do
	{
		// TODO: is this necessary given we're appending a NULL to the end of the buffer below? Possibly good practice, but
		//       but could cost us a bit in performance... Currently this code isn't used for anything performance-critical, but...
		memset(buffer, 0, sizeof(buffer));
		
		// TODO: we could also receive into a larger windowed buffer, and only do the conversion to std::string once at the end (or
		//       at least not as often).
		
		ret = ::recv(m_sock, buffer, kMaxRecvLength, 0);
		
		if (ret > 0)
		{
			buffer[ret] = '\0';
		
			data.append(buffer);
			dataLength += ret;
		}
		else
		{
			break;
		}
	}
	while (ret >= kMaxRecvLength);
	
	return dataLength > 0 ? eSockRecv_OK : eSockRecv_NoData;
}

// really hacky infrastructure to cope with Safari always sending POST params in a second TCP frame.
// This really needs to be handled in a nicer and more generic/robust way, but using a timeout recv()
// causes delays and doesn't completely fix the issue - this does (although there may still be other recv() issues).
// TODO: long-term we probably want to try and decode headers as we receive them and make the decision generically
//       based off Content-Length?
SocketRecvReturnCode Socket::recvSmart(std::string& data) const
{
	if (!isValid())
		return eSockRecv_Error;

	int dataLength = 0;

	char buffer[kMaxRecvLength + 1];

	int ret = 0;

	do
	{
		// TODO: is this necessary given we're appending a NULL to the end of the buffer below? Possibly good practice, but
		//       but could cost us a bit in performance... Currently this code isn't used for anything performance-critical, but...
		memset(buffer, 0, sizeof(buffer));

		// TODO: we could also receive into a larger windowed buffer, and only do the conversion to std::string once at the end (or
		//       at least not as often).

		ret = ::recv(m_sock, buffer, kMaxRecvLength, 0);

		if (ret > 0)
		{
			buffer[ret] = '\0';

			data.append(buffer);
			dataLength += ret;

			// check it's a POST request
			if (!StringHelpers::beginsWithStaticConst(data, "POST"))
				continue;

			// check there was no data payload
			if (!StringHelpers::endsWithStaticConst(data, "\r\n\r\n"))
				continue;

			// otherwise, we've got a POST request with no form data, so try and find the Content-Length:

			size_t contentLengthStart = data.find("Content-Length:");
			if (contentLengthStart == std::string::npos)
				continue;

			size_t contentLengthDataStart = contentLengthStart + 15;

			size_t lineEnd = data.find('\r', contentLengthDataStart);
			if (lineEnd != std::string::npos)
			{
				std::string contentLengthString = data.substr(contentLengthDataStart, lineEnd - contentLengthDataStart);

				unsigned int contentLengthVal = atoi(contentLengthString.c_str());

				if (contentLengthVal > 0)
				{
					ret = ::recv(m_sock, buffer, kMaxRecvLength, 0);

					buffer[ret] = '\0';

					data.append(buffer);
					dataLength += ret;
				}
			}
		}
		else
		{
			break;
		}
	}
	while (ret >= kMaxRecvLength);

	return dataLength > 0 ? eSockRecv_OK : eSockRecv_NoData;
}

SocketRecvReturnCode Socket::recvSmartWithTimeout(std::string& data, unsigned int timeoutSecs) const
{
	if (!isValid())
		return eSockRecv_Error;

	int dataLength = 0;

	char buffer[kMaxRecvLength + 1];

	int ret = 0;

	// check if there's anything for us with a time-out...
	struct pollfd fd;
	fd.fd = m_sock;
	fd.events = POLLIN | POLLPRI;// | POLLOUT;
	// Note: we don't need to specify POLLERR or POLLHUP, as these are automatic according
	//       to the poll() man page
	unsigned int timeoutMS = timeoutSecs * 1000; // poll() deals in ms...
	
	bool timedOut = false;
	
	unsigned int loopCount = 0;

	do
	{
		if (timeoutSecs > 0)
		{
			int pollResult = poll(&fd, 1, timeoutMS);
			if (pollResult == -1)
			{
				// there's been an error...
	//			break;
				return eSockRecv_Error;
			}
			else if (pollResult == 0)
			{
				// timed out...
				timedOut = true;
				break;
			}
			
			if (fd.revents & POLLERR)
			{
				// an error occurred, possibly the socket was closed on the other end.
				break;
			}
			else if (fd.revents & POLLHUP)
			{
				// peer closed its end of the socket
				break;
			}
		}

		// otherwise we should have data to receive...
		
		// TODO: is this necessary given we're appending a NULL to the end of the buffer below? Possibly good practice, but
		//       but could cost us a bit in performance... Currently this code isn't used for anything performance-critical, but...
		memset(buffer, 0, sizeof(buffer));

		// TODO: we could also receive into a larger windowed buffer, and only do the conversion to std::string once at the end (or
		//       at least not as often).

		ret = ::recv(m_sock, buffer, kMaxRecvLength, 0);

		if (ret > 0)
		{
			buffer[ret] = '\0';

			data.append(buffer);
			dataLength += ret;
			
			// check it's a POST request
			if (!StringHelpers::beginsWithStaticConst(data, "POST"))
				continue;

			// check there was no data payload
			if (!StringHelpers::endsWithStaticConst(data, "\r\n\r\n"))
				continue;

			// otherwise, we've got a POST request with no form data, so try and find the Content-Length:

			size_t contentLengthStart = data.find("Content-Length:");
			if (contentLengthStart == std::string::npos)
				continue;

			size_t contentLengthDataStart = contentLengthStart + 15;

			size_t lineEnd = data.find('\r', contentLengthDataStart);
			if (lineEnd != std::string::npos)
			{
				std::string contentLengthString = data.substr(contentLengthDataStart, lineEnd - contentLengthDataStart);

				unsigned int contentLengthVal = atoi(contentLengthString.c_str());

				if (contentLengthVal > 0)
				{
					ret = ::recv(m_sock, buffer, kMaxRecvLength, 0);

					buffer[ret] = '\0';

					data.append(buffer);
					dataLength += ret;
				}
			}
		}
		else
		{
			// if ret == 0, it was very likely closed by the client.
			break;
		}
		
		loopCount += 1;
	}
	while (ret >= kMaxRecvLength);
	
	if (timedOut)
	{
		return dataLength > 0 ? eSockRecv_TimedOutWithData : eSockRecv_TimedOutNoData;
	}

	return dataLength > 0 ? eSockRecv_OK : eSockRecv_NoData;
}

SocketRecvReturnCode Socket::recvWithTimeout(std::string& data, unsigned int timeoutSecs) const
{
	unsigned int dataLength = 0;
	SocketRecvReturnCode returnCode;
#if USE_POLL_FOR_RECV_TIMEOUT
	returnCode = recvWithTimeoutPoll(data, dataLength, timeoutSecs);
#else
	returnCode = recvWithTimeoutSockOpt(data, dataLength, timeoutSecs);

#endif

	return returnCode;
}

int Socket::peekRecv() const
{
	if (!isValid())
		return -1;
		
	char buffer[kMaxRecvLength + 1];
	memset(buffer, 0, sizeof(buffer));
	
	int ret = ::recv(m_sock, buffer, kMaxRecvLength, MSG_PEEK);
	
	return ret;
}

/*

bool Socket::getSocketRecvTimeout(unsigned int& timeoutSeconds) const
{
	struct timeval timeout;
	
	socklen_t optLen = sizeof(timeval);
	
	if (getsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, &optLen) != 0)
	{
		int errorNumber = errno;
		m_pLogger->error("Error getting socket recv timeout...");
		
		return false;
	}
	
	timeoutSeconds = timeout.tv_sec;
	// ignore tv_usec for the moment...
	
	return true;
}

bool Socket::setSocketRecvTimeout(unsigned int timeoutSeconds)
{
	struct timeval timeout;
	timeout.tv_sec = timeoutSeconds;
	timeout.tv_usec = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
	{
		int errorNumber = errno;
		m_pLogger->error("Error setting socket recv timeout...");
		
		return false;
	}
	
	return true;
}

*/

SocketRecvReturnCode Socket::recvWithTimeoutPoll(std::string& data, unsigned int& dataLength, unsigned int timeoutSecs) const
{
	if (!isValid())
		return eSockRecv_Error;

	dataLength = 0;

	char buffer[kMaxRecvLength + 1];

	int ret = 0;

	// check if there's anything for us with a time-out...
	struct pollfd fd;
	fd.fd = m_sock;
	fd.events = POLLIN | POLLPRI;// | POLLOUT;
	// Note: we don't need to specify POLLERR or POLLHUP, as these are automatic according
	//       to the poll() man page
	unsigned int timeoutMS = timeoutSecs * 1000; // poll() deals in ms...

	do
	{
		int pollResult = poll(&fd, 1, timeoutMS);
		if (pollResult == -1)
		{
			// there's been an error...
//			break;
			return eSockRecv_Error;
		}
		else if (pollResult == 0)
		{
			// timed out...
			break;
		}
		
		if (fd.revents & POLLERR)
		{
			// an error occurred, possibly the socket was closed on the other end.
			break;
		}
		else if (fd.revents & POLLHUP)
		{
			// peer closed its end of the socket
			break;
		}

		// otherwise we should have data to receive...
		
		// TODO: is this necessary given we're appending a NULL to the end of the buffer below? Possibly good practice, but
		//       but could cost us a bit in performance... Currently this code isn't used for anything performance-critical, but...
		memset(buffer, 0, sizeof(buffer));

		// TODO: we could also receive into a larger windowed buffer, and only do the conversion to std::string once at the end (or
		//       at least not as often).

		ret = ::recv(m_sock, buffer, kMaxRecvLength, 0);

		if (ret > 0)
		{
			buffer[ret] = '\0';

			data.append(buffer);
			dataLength += ret;
		}
		else
		{
			// if ret == 0, it was very likely closed by the client.
			break;
		}
	}
	while (ret >= kMaxRecvLength);

	return dataLength > 0 ? eSockRecv_OK : eSockRecv_NoData;
}

SocketRecvReturnCode Socket::recvWithTimeoutSockOpt(std::string& data, unsigned int& dataLength, unsigned int timeoutSecs) const
{
	if (!isValid())
		return eSockRecv_Error;

	dataLength = 0;

	char buffer[kMaxRecvLength + 1];

	int ret = 0;

	// TODO: improve this.
	// Note: using a synchronous receive timeout to perform keep-alive obviously isn't the most efficient
	//       way of doing keep-alive in terms of CPU/thread usage, but it actually works fairly well with over-subscribing
	//       the threads and SMT as long as a short timeout is set and does provide benefits over not using keep-alive.

	// Note: another issue with using SO_RCVTIMEO is that when this option is set, if the receive data length requested isn't available (very likely),
	//       instead of returning how much data there actually was as recv() does normally, it waits until it times out and then returns -1.
	//       Safari in particular seems to hate this and block for a while on the first keep-alive re-use, but Firefox and Chrome are fine.
	struct timeval timeout;
	timeout.tv_sec = timeoutSecs;
	timeout.tv_usec = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
	{
		int errorNumber = errno;
		m_pLogger->error("Error setting socket recv timeout...");
	}

	do
	{
		// TODO: is this necessary given we're appending a NULL to the end of the buffer below? Possibly good practice, but
		//       but could cost us a bit in performance... Currently this code isn't used for anything performance-critical, but...
		memset(buffer, 0, sizeof(buffer));

		// TODO: we could also receive into a larger windowed buffer, and only do the conversion to std::string once at the end (or
		//       at least not as often).

		ret = ::recv(m_sock, buffer, kMaxRecvLength, 0);

		if (ret == -1)
		{
			// no data was received, and we timed out...
			// TODO: check this is actually the case - i.e. errno == EAGAIN / EWOULDBLOCK
			break;
		}
		else if (ret > 0)
		{
			buffer[ret] = '\0';

			data.append(buffer);
			dataLength += ret;
		}
		else
		{
			// TODO: break here?
			break;
		}
	}
	while (ret >= kMaxRecvLength);

	return dataLength > 0 ? eSockRecv_OK : eSockRecv_NoData;
}
