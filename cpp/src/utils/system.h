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

#ifndef SYSTEM_H
#define SYSTEM_H

#include <string>
#include <cstdint>

class System
{
public:
	System();

	static uint16_t convertToNetworkByteOrder(uint16_t value);

	static bool downgradeUserOfProcess(const std::string& downgradeUser, bool enforceRootOriginal);
	
	static size_t getTotalMemory();
	static size_t getAvailableMemory();

	static size_t getProcessCurrentMemUsage();

	static float getLoadAverage();
};

#endif // SYSTEM_H
