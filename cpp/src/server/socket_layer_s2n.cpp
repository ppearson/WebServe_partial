/*
 WebServe
 Copyright 2019-2022 Peter Pearson.

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

#include "socket_layer_s2n.h"

#if WEBSERVE_ENABLE_HTTPS_SUPPORT
#include <s2n.h>
#include <signal.h>
#endif

#include <unistd.h>
#include <poll.h>

#include "web_server_common.h"

#include "utils/file_helpers.h"
#include "utils/string_helpers.h"

static const unsigned int kMaxRecvLengthS2N = 4096;

ConnectionSocketS2N::ConnectionSocketS2N(Logger& logger, Socket* pRawSocket, struct s2n_connection* pS2NConnection, bool ownConnection)
		: ConnectionSocket(logger),
			m_active(true),
			m_pRawSocket(pRawSocket),
			m_ownConnection(ownConnection)
{
	m_pS2NConnection = pS2NConnection;
}

ConnectionSocketS2N::~ConnectionSocketS2N()
{
	close(false);
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_ownConnection)
	{
		// only free it if we own it - i.e. it was allocated specifically for us, not a shared re-usable one...
		s2n_connection_free(m_pS2NConnection); // this currently only ever returns 0
	}
	
	m_pS2NConnection = nullptr;
#endif
}

bool ConnectionSocketS2N::send(const std::string& data, unsigned int flags) const
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	s2n_blocked_status blocked;
	
	const char* pBuffer = data.c_str();
	size_t bytesRemaining = data.size();
	ssize_t bytesSent = 0;
	do
	{
		bytesSent = s2n_send(m_pS2NConnection, pBuffer, bytesRemaining, &blocked);
		if (bytesSent < 0)
		{
			int localErrno = errno;
			
			if (flags & SEND_IGNORE_FAILURES)
				return false;
			
			// ignore printing error for 104 - connection reset by peer, as well as 32 - EPIPE
			if (localErrno != ECONNRESET && localErrno != EPIPE)
			{
				m_logger.debug("Error writing to connection: '%s'", s2n_strerror(s2n_errno, "EN"));
			}
			return false;
		}

		bytesRemaining -= bytesSent;
		pBuffer += bytesSent;
	}
	while (bytesRemaining || blocked);
	
	return bytesRemaining == 0;
#else
	return false;	
#endif
}

bool ConnectionSocketS2N::send(unsigned char* pData, size_t dataLength) const
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	s2n_blocked_status blocked;
	
	const unsigned char* pBuffer = pData;
	size_t bytesRemaining = dataLength;
	ssize_t bytesSent = 0;
	do
	{
		bytesSent = s2n_send(m_pS2NConnection, pBuffer, bytesRemaining, &blocked);
		if (bytesSent < 0)
		{
			int localErrno = errno;
			// ignore printing error for 104 - connection reset by peer, as well as 32 - EPIPE
			if (localErrno != ECONNRESET && localErrno != EPIPE)
			{
				m_logger.debug("Error writing to connection: '%s'", s2n_strerror(s2n_errno, "EN"));
			}
			return false;
		}

		bytesRemaining -= bytesSent;
		pBuffer += bytesSent;
	}
	while (bytesRemaining || blocked);
	
	return bytesRemaining == 0;
#else
	return false;
#endif
}

SocketRecvReturnCode ConnectionSocketS2N::recv(std::string& data) const
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	s2n_blocked_status blocked;

	char buffer[kMaxRecvLengthS2N];
	int bytesRead = 0;
	do
	{
		bytesRead = s2n_recv(m_pS2NConnection, buffer, kMaxRecvLengthS2N, &blocked);
		if (bytesRead == 0)
		{
			// TODO: is this correct?
			return SocketRecvReturnCode(eSockRecv_NoData);
		}
		if (bytesRead < 0)
		{
			m_logger.debug("Error reading from S2N connection: '%s' %d", s2n_strerror(s2n_errno, "EN"), s2n_connection_get_alert(m_pS2NConnection));
			return SocketRecvReturnCode(eSockRecv_Error);
		}
		
		buffer[bytesRead] = '\0';
	
		data.append(buffer);
	}
	while (blocked);
	
	return SocketRecvReturnCode(eSockRecv_OK);
#else
	return SocketRecvReturnCode(eSockRecv_Error);
#endif
}

SocketRecvReturnCode ConnectionSocketS2N::recvSmart(std::string& data, unsigned int timeoutSecs) const
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	s2n_blocked_status blocked;
	
	int dataLength = 0;

	char buffer[kMaxRecvLengthS2N + 1];

	int bytesRead = 0;
	
	unsigned int loopCount = 0;
	
	// check if there's anything for us with a time-out...
	struct pollfd fd;
	fd.fd = m_pRawSocket->getSocketFD();
	fd.events = POLLIN | POLLPRI;// | POLLOUT;
	// Note: we don't need to specify POLLERR or POLLHUP, as these are automatic according
	//       to the poll() man page
	unsigned int timeoutMS = timeoutSecs * 1000; // poll() deals in ms...
	
	bool timedOut = false;

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

		bytesRead = s2n_recv(m_pS2NConnection, buffer, kMaxRecvLengthS2N, &blocked);
		if (bytesRead == 0)
		{
			// TODO: is this right?
			return SocketRecvReturnCode(eSockRecv_NoData);
		}
		if (bytesRead < 0)
		{
			int localErrno = errno;
			if (localErrno != ECONNRESET && localErrno != EPIPE)
			{
				m_logger.debug("Error reading from S2N connection: '%s' %d", s2n_strerror(s2n_errno, "EN"), s2n_connection_get_alert(m_pS2NConnection));
				return SocketRecvReturnCode(eSockRecv_Error);
			}
			else
			{
				// otherwise, we silently fail
				return SocketRecvReturnCode(eSockRecv_PeerClosed);
			}
		}

		// this check is hopefully not needed given the above, but..
		if (bytesRead > 0)
		{
			buffer[bytesRead] = '\0';

			data.append(buffer);
			dataLength += bytesRead;

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
					bytesRead = s2n_recv(m_pS2NConnection, buffer, kMaxRecvLengthS2N, &blocked);
					
					if (bytesRead > 0)
					{
						buffer[bytesRead] = '\0';
	
						data.append(buffer);
						dataLength += bytesRead;
					}
				}
			}
		}
		else
		{
			break;
		}
		
		loopCount += 1;
	}
	while (blocked);
	
	if (timedOut)
	{
		return dataLength > 0 ? eSockRecv_TimedOutWithData : eSockRecv_TimedOutNoData;
	}

	return (dataLength > 0) ? SocketRecvReturnCode(eSockRecv_OK) : SocketRecvReturnCode(eSockRecv_NoData);
#else
	return SocketRecvReturnCode(eSockRecv_Error);
#endif
}

SocketRecvReturnCode ConnectionSocketS2N::recvWithTimeout(std::string& data, unsigned int timeoutSecs) const
{
	unsigned int dataLength = 0;
	SocketRecvReturnCode retCode = recvWithTimeoutPoll(data, dataLength, timeoutSecs);

	return retCode;
}

void ConnectionSocketS2N::accumulateSocketConnectionStatistics(ConnectionStatistics& connStatistics) const
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	connStatistics.httpsBytesReceived += s2n_connection_get_wire_bytes_in(m_pS2NConnection);
	connStatistics.httpsBytesSent += s2n_connection_get_wire_bytes_out(m_pS2NConnection);
#endif
}

bool ConnectionSocketS2N::close(bool deleteRawSocket)
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_active)
	{
		m_logger.debug("Closing S2N connection");
		
		s2n_blocked_status blocked;
		int res = s2n_shutdown(m_pS2NConnection, &blocked);
		if (res != 0)
		{
			// try again, as that sometimes helps (locally, anyway)...
			::sleep(1);
			res = s2n_shutdown(m_pS2NConnection, &blocked);
			if (res != 0)
			{
				// this check is somewhat pointless, but...
				if (blocked == S2N_BLOCKED_ON_READ)
				{
					int errorType = s2n_error_get_type(s2n_errno);
					// if it's closed already, don't bother erroring...
					if (errorType != S2N_ERR_T_CLOSED)
					{
						if (errorType == S2N_ERR_T_BLOCKED)
						{
							m_logger.debug("Error shutting down S2N socket connection, it's blocked on read, with s2n_errno: %i, s2nerror: %s", s2n_errno, s2n_strerror(s2n_errno, "EN"));
						}
						else
						{
							// not really even sure this can happen (other error than blocked), but...
							m_logger.error("Error shutting down S2N socket connection, with s2n_errno: %i, s2nerror: %s", s2n_errno, s2n_strerror(s2n_errno, "EN"));
						}
					}
				}
//				m_logger.error("Error shutting down S2N socket connection.");
			}
		}

		// S2N doesn't seem to free all memory in s2n_connection_free() call (Destructor above)
		// unless this is also called, and we want to do this anyway when we re-use connections...
		if (s2n_connection_wipe(m_pS2NConnection) < 0)
		{
			m_logger.error("Error wiping S2N connection");
		}
		m_active = false;
	}
#endif
	if (m_pRawSocket)
	{
		m_pRawSocket->close();
	}
	
	// we don't really own this, but it's convenient doing this this way...
	if (deleteRawSocket)
	{
		delete m_pRawSocket;
		m_pRawSocket = nullptr;
	}
	
	return true;
}

SocketRecvReturnCode ConnectionSocketS2N::recvWithTimeoutPoll(std::string& data, unsigned int& dataLength, unsigned int timeoutSecs) const
{
	if (!m_pRawSocket->isValid())
		return SocketRecvReturnCode(eSockRecv_Error);

#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	s2n_blocked_status blocked;

	char buffer[kMaxRecvLengthS2N + 1];

	int ret = 0;

	// check if there's anything for us with a time-out...
	struct pollfd fd;
	fd.fd = m_pRawSocket->getSocketFD();
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
			return SocketRecvReturnCode(eSockRecv_Error);
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

		ret = s2n_recv(m_pS2NConnection, buffer, kMaxRecvLengthS2N, &blocked);

		if (ret == 0)
		{
			// TODO: is this correct?
			return SocketRecvReturnCode(eSockRecv_NoData);
		}
		if (ret < 0)
		{
			int localError = errno;
			m_logger.debug("Error reading from S2N connection: '%s' %d", s2n_strerror(s2n_errno, "EN"), s2n_connection_get_alert(m_pS2NConnection));
			return SocketRecvReturnCode(eSockRecv_Error);
		}
		
		buffer[ret] = '\0';
	
		data.append(buffer);
		
		dataLength += ret;
	}
	while (ret >= kMaxRecvLengthS2N || blocked);

	return (ret > 0) ? SocketRecvReturnCode(eSockRecv_OK) : SocketRecvReturnCode(eSockRecv_NoData);
#else
	return SocketRecvReturnCode(eSockRecv_Error);
#endif
}

//

SocketLayerS2N::SocketLayerS2N(Logger& logger) : SocketLayer(logger)
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	m_s2nConfig = nullptr;
#endif
}

SocketLayerS2N::~SocketLayerS2N()
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_s2nConfig)
	{	
		s2n_config_free(m_s2nConfig);
		m_s2nConfig = nullptr;
		
		s2n_cleanup();
	}
#endif
}

bool SocketLayerS2N::configure(const Configuration& configuration)
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (configuration.isHTTPSv4Enabled())
	{
		// because we don't control the sockets directly, we can't pass MSG_NOSIGNAL to stop
		// SIGPIPE happening, so we have to disable it at this level.
		
		{
			struct sigaction act;
			memset(&act, 0, sizeof(act));
			act.sa_handler = SIG_IGN;
			act.sa_flags = SA_RESTART;
			int retVal = sigaction(SIGPIPE, &act, NULL);
			if (retVal)
			{
				m_logger.error("Error disabling SIGPIPE. It's very likely connections won't work reliably.");
			}
		}
		
		setenv("S2N_DONT_MLOCK", "1", 1);
		
		int res = s2n_init();
	
		m_s2nConfig = s2n_config_new();
	 
		if (!m_s2nConfig)
		{
			m_logger.error("Could not create s2n config.");
			return false;
		}
	 
		// TODO: certificate stuff
	 
	
		// load certificates - we can have multiple of each, in pairs of cert/key paths.
		
		// A key assumption here for using multiple is that the pairs will be described in the
		// config file as cert1, key1, cert2, key2, otherwise this logic won't work.
		
		// If there are ANY errors at all with this stuff, we abort startup with a critical error
		const std::vector<std::string>& certPaths = configuration.getHTTPSCertificatePaths();
		const std::vector<std::string>& keyPaths = configuration.getHTTPSKeyPaths();
		
		if (!certPaths.empty() && certPaths.size() == keyPaths.size())
		{
			unsigned int numPairs = certPaths.size();
			
			for (unsigned int i = 0; i < numPairs; i++)
			{
				const std::string& certPath = certPaths[i];
				const std::string& keyPath = keyPaths[i];
				
				m_logger.info("certPath: %s, keyPath: %s", certPath.c_str(), keyPath.c_str());
				
				std::string certText = FileHelpers::getFileTextContent(certPath);
				std::string keyText = FileHelpers::getFileTextContent(keyPath);
				
				if (!certText.empty() && !keyText.empty())
				{
					struct s2n_cert_chain_and_key* pChainAndKey = s2n_cert_chain_and_key_new();
					if (s2n_cert_chain_and_key_load_pem(pChainAndKey, certText.c_str(), keyText.c_str()) < 0)
					{
						m_logger.error("Error loading certificate or key.");
						return false;
					}
			
					if (s2n_config_add_cert_chain_and_key_to_store(m_s2nConfig, pChainAndKey) < 0)
					{
						m_logger.error("Error setting certificate or key.");
						return false;
					}
				}
				else
				{
					// TODO: make this more specific and helpful
					m_logger.critical("Can't load certificate or key from specified files.");
					return false;
				}
			}
			
			m_logger.notice("Successfully loaded %u cert/key %s.", numPairs, (numPairs == 1) ? "pair" : "pairs");
		}
		else
		{
			bool noCert = certPaths.empty();
			bool noKey = keyPaths.empty();
			std::string errorMsg;
			if (noCert && noKey)
			{
				errorMsg = "neither a certificate or a key filename were specified in the config file";
			}
			else
			{
				errorMsg = "no " + ((noCert) ? std::string("certificate") : std::string("key")) + " filename was specified in the config file";
			}
			m_logger.critical("HTTPS support was enabled, but %s.", errorMsg);
			return false;
		}

		// S2N_CERT_AUTH_REQUIRED
		if (s2n_config_set_client_auth_type(m_s2nConfig, S2N_CERT_AUTH_NONE) < 0)
		{
			m_logger.error("Error settting s2n client auth type...");
			return false;
		}
		
		if (s2n_config_set_cipher_preferences(m_s2nConfig, "default_tls13") < 0)
		{
			m_logger.error("Could not set s2n config cipher preferences: '%s'. %s", s2n_strerror(s2n_errno, "EN"), s2n_strerror_debug(s2n_errno, "EN"));
			return false;
		}
		
		m_logger.info("Configured S2N for HTTPS use.");
		return true;
	}
#endif

	return false;	
}

SocketLayerThreadContext* SocketLayerS2N::allocatePerThreadContext()
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	S2NSocketLayerThreadContext* pNewThreadContext = new S2NSocketLayerThreadContext();
	
	struct s2n_connection* conn = s2n_connection_new(S2N_SERVER);
	
	if (!conn)
	{
		m_logger.error("Error creating new s2n connection for per thread context...");
		return nullptr;
	}
	
	if (s2n_connection_set_config(conn, m_s2nConfig) < 0)
	{
		m_logger.error("Could not set s2n socket config for per thread context.");
		return nullptr;
	}
	
	s2n_connection_set_blinding(conn, S2N_SELF_SERVICE_BLINDING);
	
//	s2n_connection_prefer_throughput(conn);
	s2n_connection_prefer_low_latency(conn);
	
	pNewThreadContext->m_s2nConnection = conn;
	
	return pNewThreadContext;
#else
	return nullptr;
#endif
}

ReturnCodeType SocketLayerS2N::allocateSpecialisedConnectionSocket(RequestConnection& connection)
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	
	bool ownThisConnection = false;
	
	struct s2n_connection* conn = nullptr;
	
	if (connection.pThreadConfig->pSLThreadContext)
	{
		// we should have a cached s2n_connection already we can re-use
		
		S2NSocketLayerThreadContext* pLocalSLTC = static_cast<S2NSocketLayerThreadContext*>(connection.pThreadConfig->pSLThreadContext);
		
		if (pLocalSLTC->m_s2nConnection)
		{
			conn = pLocalSLTC->m_s2nConnection;
		}
	}
	
	if (conn == nullptr)
	{
		// we don't have a cached one, so create a dedicated one for this connection
		ownThisConnection = true;
		
		conn = s2n_connection_new(S2N_SERVER);
		
		if (!conn)
		{
			m_logger.error("Error creating new s2n connection...");
			return eReturnFail;
		}
		
		// we only need to set this on new connections, as internally, s2n_connection_setconfig()
		// early-outs if the config is the same...
		if (s2n_connection_set_config(conn, m_s2nConfig) < 0)
		{
			m_logger.error("Could not set s2n socket config.");
			return eReturnFail;
		}
	}
	
	// currently, we need to set these for both new and re-used connections
	// as s2n_connection_wipe() seems to not keep them around...
	s2n_connection_set_blinding(conn, S2N_SELF_SERVICE_BLINDING);
	
//	s2n_connection_prefer_throughput(conn);
	s2n_connection_prefer_low_latency(conn);
	
//	s2n_set_server_name(conn, )
		
	// try and set the socket fd
	if (s2n_connection_set_fd(conn, connection.pRawSocket->getSocketFD()) < 0)
	{
		m_logger.error("Couldn't set s2n connection socket fd.");
		return eReturnFail;
	}
	
	bool pollFirst = true;
	bool hadPollRecvData = false;
	unsigned int negCount = 0;
	bool peekFirst = true;
	int peekSize = 0;
	
	bool setRecvTimeout = true;
	
	// negotiate
	s2n_blocked_status blocked;
	do
	{
		if (pollFirst)
		{
			// check there's something to receive first.
			
			hadPollRecvData = false;
			
			// check if there's anything for us with a time-out...
			struct pollfd fd;
			fd.fd = connection.pRawSocket->getSocketFD();
			fd.events = POLLIN | POLLPRI;// | POLLOUT;
			// Note: we don't need to specify POLLERR or POLLHUP, as these are automatic according
			//       to the poll() man page
			unsigned int timeoutMS = 8 * 1000; // poll() deals in ms...
		
			int pollResult = poll(&fd, 1, timeoutMS);
			if (pollResult == -1)
			{
				// there's been an error...
				m_logger.error("poll() failed for s2n connection negotiation.");
				return eReturnFail;
			}
			else if (pollResult == 0)
			{
				// timed out...
				m_logger.debug("s2n connection negotiation poll() timed out.");
				return eReturnFailSilent;
			}
			
			if (fd.revents & POLLERR)
			{
				// an error occurred, possibly the socket was closed on the other end.
				return eReturnFailSilent;
			}
			else if (fd.revents & POLLHUP)
			{
				// peer closed its end of the socket
				return eReturnFailSilent;
			}
			else if (fd.revents & POLLIN || fd.revents & POLLPRI)
			{
				// there was data to read
				hadPollRecvData = true;
			}
		}
		
		if (peekFirst)
		{
			peekSize = connection.pRawSocket->peekRecv();
			// TSL header is 5 bytes
			if (peekSize < 5)
			{
				m_logger.debug("s2n connection negotiation recv() peek returned less than 5 bytes.");
				return eReturnFailSilent;
			}
		}
		
		negCount += 1;
		
		if (setRecvTimeout)
		{
			int timeoutSecs = 5;
			if (!connection.pRawSocket->setRecvTimeoutOption(timeoutSecs))
			{
				m_logger.debug("Failed to set socket recv timeout before s2n_negotiate.");
			}
		}
		
		if (s2n_negotiate(conn, &blocked) < 0)
		{
			int localErrno = errno;
			
			// we often seem to get errno 64 (ENONET) for some reason...
			// so ignore that for the moment in terms of printing an error
			
			// also ignore 32 (EPIPE, socket closed at other end), which often happens when browsers close the sockets
			// as they don't need them...
			
			bool canContinue = true;
			
			if (localErrno != ENONET && localErrno != EPIPE && localErrno != ECONNRESET &&
					localErrno != 0)
			{
				// only bother logging situations when s2n would have blocked to receive in verbose logging...
				if (s2n_error_get_type(s2n_errno) == S2N_ERR_T_BLOCKED)
				{
					m_logger.debug("Failed to negotiate s2n connection: '%s'. %s", s2n_strerror(s2n_errno, "EN"), s2n_strerror_debug(s2n_errno, "EN"));
				}
				else
				{
					canContinue = false;
					m_logger.error("Failed to negotiate s2n connection: '%s'. %s", s2n_strerror(s2n_errno, "EN"), s2n_strerror_debug(s2n_errno, "EN"));
				}
			}
			
			if (setRecvTimeout && !connection.pRawSocket->setRecvTimeoutOption(0))
			{
				m_logger.debug("Failed to set socket recv timeout before s2n_negotiate.");
			}
			
			// TODO: free up socket and connection if we own them?
			
			s2n_blocked_status blocked2;
			s2n_shutdown(conn, &blocked2);
		
			// S2N doesn't seem to free all memory in s2n_connection_free() call (Destructor above)
			// unless this is also called, and we want to do this anyway when we re-use connections...
			if (s2n_connection_wipe(conn) < 0)
			{
				m_logger.error("Error wiping S2N connection");
			}
			
			return canContinue ? eReturnFailSilent : eReturnFail;
		}
	}
	while (blocked);
	
	// specify that the connection is securely authenticated
	connection.https = true;
	
	if (setRecvTimeout && !connection.pRawSocket->setRecvTimeoutOption(0))
	{
		m_logger.debug("Failed to set socket recv timeout before s2n_negotiate.");
	}
	
	m_logger.debug("Successfully negotiated s2n connection.");
			
	connection.pConnectionSocket = new ConnectionSocketS2N(m_logger, connection.pRawSocket, conn, ownThisConnection);
	
	return eReturnOK;
#else
	return eReturnFail;
#endif	
}

SocketLayerS2N::S2NSocketLayerThreadContext::~S2NSocketLayerThreadContext()
{
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_s2nConnection)
	{
		s2n_connection_free(m_s2nConnection);
	}
#endif
}
