/*
 WebServe
 Copyright 2018-2020 Peter Pearson.

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

#include "system.h"

#include <cstdio>

#include <memory>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef _MSC_VER
#if __APPLE__
#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#else
// linux / unix
#endif
#else
#include <windows.h>
#endif

System::System()
{

}

uint16_t System::convertToNetworkByteOrder(uint16_t value)
{
	value = (value >> 8) | (value << 8);

	return value;
}

bool System::downgradeUserOfProcess(const std::string& downgradeUser, bool enforceRootOriginal)
{
	// TODO: do we want geteuid() or getuid() here? In testing on OS X they appear to return the same things for various users...

	// getuid() returns 0 if root, so we can use this to make sure we are root before doing anything...

	// TODO: although do we want to fail in this situation? We can't change to the actual user requested as we're not root,
	//       but we could see if we *are* that user already and fail gracefully...

	if (downgradeUser.empty())
		return false;

	// get hold of the uid of the downgrade username
	struct passwd pwd;
	struct passwd* pwdResult = nullptr;
	size_t pwdBuffSize = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (pwdBuffSize == -1u)
	{
		pwdBuffSize = 16384;
	}
	std::unique_ptr<char[]> buffer(new char[pwdBuffSize]);
	if (!buffer)
	{
		return false;
	}

	uid_t targetUID = -1;
	gid_t targetGroupID = -1;

	int retCode = getpwnam_r(downgradeUser.c_str(), &pwd, buffer.get(), pwdBuffSize, &pwdResult);
	if (pwdResult == nullptr)
	{
		if (retCode == 0)
		{
			// user wasn't found
			return false;
		}

		return false;
	}
	else
	{
		if (retCode == 0)
		{
			targetUID = pwdResult->pw_uid;
			targetGroupID = pwdResult->pw_gid;
		}
		else
		{
			// unknown error
			return false;
		}
	}

	uid_t currentUID = getuid();
	if (currentUID)
	{
		// we're already a user which isn't root, so if we don't need to enforce root as being the original user,
		// allow it through...
		if (currentUID == targetUID && !enforceRootOriginal)
		{
			// we're already that user, so for the moment, don't do anything and pretend we worked by returning true
			// if we don't require root to be the original user.
			return true;
		}
		return false;
	}
	
	// first of all try setting group id
	if (targetGroupID != -1)
	{
		if (setgid(targetGroupID) == -1)
		{
			return false;
		}
		if (setgroups(0, NULL) == -1)
		{
			return false;
		}
		if (initgroups(downgradeUser.c_str(), targetGroupID) == -1)
		{
			return false;
		}
	}

	if (setuid(targetUID) == -1)
	{
		// TODO: error handling...
//		fprintf(stderr, "Failed to downgrade user.\n");
		return false;
	}
	
	// TODO: we probably also want to change the groupid as well?

	return true;
}

size_t System::getTotalMemory()
{
	size_t total = 0;
#if __APPLE__
	int64_t memory = 0;
	size_t size = sizeof(memory);
	int mib[2];
	mib[0] = CTL_HW;
	mib[1] = HW_MEMSIZE;
	if (sysctl(mib, 2, &memory, &size, nullptr, 0) != -1)
	{
		total = memory;
	}
#else
	size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);
	size_t totalPages = (size_t)sysconf(_SC_PHYS_PAGES);

	total = pageSize * totalPages;
#endif
	return total;
}

size_t System::getAvailableMemory()
{
	size_t available = 0;
#if __APPLE__
	// get page size first
	int64_t pageSize = 0;
	size_t size = sizeof(pageSize);
	int mib[2];
	mib[0] = CTL_HW;
	mib[1] = HW_PAGESIZE;
	if (sysctl(mib, 2, &pageSize, &size, nullptr, 0) == -1)
	{
		return 0;
	}

	kern_return_t kret;
	vm_statistics_data_t stats;
	unsigned int numBytes = HOST_VM_INFO_COUNT;
	kret = host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&stats, &numBytes);
	if (kret == KERN_SUCCESS)
	{
		available = stats.free_count * pageSize;
	}
#else
	size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);
	size_t availablePages = (size_t)sysconf(_SC_AVPHYS_PAGES);

	available = pageSize * availablePages;
#endif
	return available;
}

size_t System::getProcessCurrentMemUsage()
{
	size_t memSize = 0;

#if __APPLE__
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS)
	{
		memSize = (size_t)info.resident_size;
	}
#else
	FILE* pFile = fopen("/proc/self/statm", "r");
	if (!pFile)
		return false;

	unsigned long rssSize = 0;
	// ignore first item up to whitespace
	fscanf(pFile, "%*s%ld", &rssSize);

	size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);
	memSize = rssSize * pageSize;

	fclose(pFile);
#endif

	return memSize;
}

float System::getLoadAverage()
{
	return 1.0f;
}
