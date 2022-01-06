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

#include "string_table.h"

#include <cstring>

#include "memory.h"

StringTable::StringTable() :
	m_pCurrentBlock(nullptr)
{

}

StringTable::~StringTable()
{
	freeMem();
}

void StringTable::init(size_t blockSize)
{
	freeMem();

	m_blockSize = blockSize;
	m_currentBlockPos = 0;
	
	m_pCurrentBlock = allocAligned<char>(blockSize);
}

StringInstance StringTable::createString(const std::string& stringValue)
{
	if (stringValue.empty())
	{
		return StringInstance(nullptr, 0);
	}

	Hash hasher;
	hasher.addString(stringValue);

	HashValue hValue = hasher.getHash();

	auto itFind = m_aStrings.find(hValue);

	if (itFind == m_aStrings.end())
	{
		// we don't have it yet, so allocate it

		const char* pNewString = allocString(stringValue);

		m_aStrings[hValue] = pNewString;

		return StringInstance(pNewString, hValue);
	}
	else
	{
		// TODO!!!: We really need to check this (we're not using it *that* much currently),
		//          as it's very likely there could in theory be hash collisions, so we likely
		//          need to do additional string checks (char-by-char cmp) as well as this...
		
		// we have it already, so just return a new StringInstance pointing at the address...

		const char* pExistingString = itFind->second;

		return StringInstance(pExistingString, hValue);
	}
}

void StringTable::freeMem()
{
	if (m_pCurrentBlock)
	{
		freeAligned(m_pCurrentBlock);
		m_pCurrentBlock = nullptr;
	}

	for (size_t i = 0; i < m_usedBlocks.size(); i++)
		freeAligned(m_usedBlocks[i]);

	m_usedBlocks.clear();

	m_currentBlockPos = 0;
}

const char* StringTable::allocString(const std::string& stringValue)
{
	size_t stringLength = stringValue.size() + 1; // need to add null terminator

	char* newCharString = (char*)alloc(stringLength);

	strcpy(newCharString, stringValue.c_str());
	newCharString[stringLength - 1] = 0;

	return (const char*)newCharString;
}

void* StringTable::alloc(size_t size)
{
	// round up to nearest 16
	size = ((size + 15) & (~15));

	if (m_currentBlockPos + size > m_blockSize)
	{
		if (m_pCurrentBlock)
		{
			m_usedBlocks.push_back(m_pCurrentBlock);
		}
		m_pCurrentBlock = allocAligned<char>(std::max(size, m_blockSize));
		m_currentBlockPos = 0;
	}

	void* ret = m_pCurrentBlock + m_currentBlockPos;
	m_currentBlockPos += size;
	return ret;
}
