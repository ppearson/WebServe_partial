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

#include "status_service.h"

#include "utils/system.h"
#include "utils/string_helpers.h"

#include "web_server_common.h"

StatusService::StatusService() : m_pTimerThread(nullptr),
	m_active(false),
	m_httpConnectionsCount(0),
	m_httpsConnectionsCount(0),
	m_httpRequestsCount(0),
	m_httpsRequestsCount(0),
	m_httpBytesReceived(0),
	m_httpBytesSent(0),
	m_httpsBytesReceived(0),
	m_httpsBytesSent(0)
{
	
}

StatusService::~StatusService()
{
	// note: we purposefully leak the thread here for the moment...
}

void StatusService::start()
{
	m_active = true;
	
	m_startTime = std::chrono::steady_clock::now();
	m_pTimerThread = new std::thread(&StatusService::timerThreadFunction, this);
}

void StatusService::stop()
{
	m_active = false;
//	m_pTimerThread->join();
}

std::string StatusService::getCurrentStatusHTML()
{
	std::string html;
	
	std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
	
	double secondsUptime = std::chrono::duration_cast<std::chrono::seconds>(timeNow - m_startTime).count();
	
	size_t freeMem = System::getAvailableMemory();
	size_t webServeRSS = System::getProcessCurrentMemUsage();
	
	html += StringHelpers::formatTimePeriod(secondsUptime) + " uptime.<br><br>\n";
	
	html += "<table>\n";
	
	html += "<tr><td>HTTP connections:</td><td>" + StringHelpers::formatNumberThousandsSeparator(m_httpConnectionsCount) + "</td></tr>\n";
	html += "<tr><td>HTTP requests:</td><td>" + StringHelpers::formatNumberThousandsSeparator(m_httpRequestsCount) + "</td></tr>\n";
	html += "<tr><td>HTTP Bytes Received:</td><td>" + StringHelpers::formatNumberThousandsSeparator(m_httpBytesReceived) + "</td></tr>\n";
	html += "<tr><td>HTTP Bytes Sent:</td><td>" + StringHelpers::formatNumberThousandsSeparator(m_httpBytesSent) + "</td></tr>\n";
	
	// bank row
	html += "<tr><td colspan=\"2\"></td><tr>\n";
	
	html += "<tr><td>HTTPS connections:</td><td>" + StringHelpers::formatNumberThousandsSeparator(m_httpsConnectionsCount) + "</td></tr>\n";
	html += "<tr><td>HTTPS requests:</td><td>" + StringHelpers::formatNumberThousandsSeparator(m_httpsRequestsCount) + "</td></tr>\n";	
	html += "<tr><td>HTTPS Bytes Received:</td><td>" + StringHelpers::formatNumberThousandsSeparator(m_httpsBytesReceived) + "</td></tr>\n";
	html += "<tr><td>HTTPS Bytes Sent:</td><td>" + StringHelpers::formatNumberThousandsSeparator(m_httpsBytesSent) + "</td></tr>\n";
	
	// bank row
	html += "<tr><td colspan=\"2\"></td><tr>\n";
	
	html += "<tr><td>Free memory:</td><td>" + StringHelpers::formatSize(freeMem) + "</td></tr>\n";
	html += "<tr><td>WebServe RSS:</td><td>" + StringHelpers::formatSize(webServeRSS) + "</td></tr>\n";
	
	html += "</table>\n<br>\n";
	
	html += "History:<br>\n";
	
	m_snapshotLock.lock();
	
	std::vector<StatusSnapshot> snapshotsCopy = m_aSnapshots;
	
	m_snapshotLock.unlock();
	
	html += "<table>\n";
	
	char szTemp[128];
	
	html += "<tr><td>TS</td><td width=\"100\">RSS</td><td width=\"120\">Connections</td><td width=\"120\">Bytes Received</td><td width=\"120\">Bytes Sent</td></tr>\n";
	
	struct tm* tmVal;
	for (const StatusSnapshot& ss : snapshotsCopy)
	{
		html += "<tr><td>";
		tmVal = localtime(&ss.timeVal);
		strftime(szTemp, 128, "%Y-%m-%d %H:%M:%S", tmVal);
		
		html.append(szTemp);
		
		html += "</td><td>" + StringHelpers::formatSize(ss.webServeRSS);
		html += "</td><td>" + StringHelpers::formatNumberThousandsSeparator(ss.httpsConnectionsCount);
		html += "</td><td>" + StringHelpers::formatSize(ss.httpsBytesReceived);
		html += "</td><td>" + StringHelpers::formatSize(ss.httpsBytesSent);
		
		html += "</td></tr>\n";
	}
	
	html += "</table>\n";
	
	return html;
}

void StatusService::accumulateConnectionStatistics(const ConnectionStatistics& connStatistics)
{
	if (!m_active)
		return;
	
	m_httpConnectionsCount += connStatistics.httpConnections;
	m_httpsConnectionsCount += connStatistics.httpsConnections;
	
	m_httpRequestsCount += connStatistics.httpRequests;
	m_httpsRequestsCount += connStatistics.httpsRequests;
	
	m_httpBytesReceived += connStatistics.httpBytesReceived;
	m_httpBytesSent += connStatistics.httpBytesSent;
	
	m_httpsBytesReceived += connStatistics.httpsBytesReceived;
	m_httpsBytesSent += connStatistics.httpsBytesSent;
}

void StatusService::timerThreadFunction()
{
	while (m_active)
	{
		performStatusSnapshot();		
		
		std::this_thread::sleep_for(std::chrono::hours(2));
	}
}

void StatusService::performStatusSnapshot()
{
	StatusSnapshot newSnapshot(time(NULL), System::getAvailableMemory(), System::getProcessCurrentMemUsage());
	
	newSnapshot.httpsConnectionsCount = m_httpsConnectionsCount.load();
	newSnapshot.httpsRequestsCount = m_httpsRequestsCount.load();
	newSnapshot.httpsBytesReceived = m_httpsBytesReceived.load();
	newSnapshot.httpsBytesSent = m_httpsBytesSent.load();
	
	m_snapshotLock.lock();
	
	m_aSnapshots.emplace_back(newSnapshot);
	
	m_snapshotLock.unlock();
}
