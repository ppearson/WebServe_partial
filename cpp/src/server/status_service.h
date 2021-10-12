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

#ifndef STATUSSERVICE_H
#define STATUSSERVICE_H

#include <thread>
#include <atomic>
#include <condition_variable>
#include <ctime>
#include <vector>

struct ConnectionStatistics;

class StatusService
{
public:
	StatusService();
	~StatusService();
	
	void start();
	void stop();
	
	std::string getCurrentStatusHTML();
	
	void accumulateConnectionStatistics(const ConnectionStatistics& connStatistics);
	
protected:
	void timerThreadFunction();
	
	void performStatusSnapshot();
	
	struct StatusSnapshot
	{
		StatusSnapshot(std::time_t timeV, size_t freeMem, size_t wsRSS) : 
			timeVal(timeV), availableFreeMemory(freeMem), webServeRSS(wsRSS)
		{
			
		}

		std::time_t timeVal;
		
		size_t		availableFreeMemory;
		size_t		webServeRSS;
		
		uint64_t	httpsConnectionsCount = 0;
		uint64_t	httpsRequestsCount = 0;
		
		uint64_t	httpsBytesReceived = 0;
		uint64_t	httpsBytesSent = 0;
	};
	
protected:
	// this is by design a pointer which leaks, until we either work out how to abort
	// the timer or have much shorter sleeps (which might be necessary to do)
	std::thread*					m_pTimerThread;
	
	bool							m_active;
	
	std::atomic<uint64_t>			m_httpConnectionsCount;
	std::atomic<uint64_t>			m_httpsConnectionsCount;
	
	std::atomic<uint64_t>			m_httpRequestsCount;
	std::atomic<uint64_t>			m_httpsRequestsCount;
	
	std::atomic<uint64_t>			m_httpBytesReceived;
	std::atomic<uint64_t>			m_httpBytesSent;
	
	std::atomic<uint64_t>			m_httpsBytesReceived;
	std::atomic<uint64_t>			m_httpsBytesSent;
	
	std::chrono::steady_clock::time_point	m_startTime;
	
	std::mutex						m_snapshotLock;
	std::vector<StatusSnapshot>		m_aSnapshots;
};

#endif // STATUSSERVICE_H
