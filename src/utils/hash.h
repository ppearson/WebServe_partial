/*
 WebServe
 Copyright 2019 Peter Pearson.
 Orginally taken from:
 Imagine
 Copyright 2011-2015 Peter Pearson.

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

#ifndef HASH_H
#define HASH_H

#include <string>

typedef unsigned long long HashValue;

class Hash
{
public:
	Hash();

	HashValue getHash() const { return m_hash; }

	// these are designed to be used in CRC style, only to detect differences in state
	void addInt(int value);
	void addUInt(unsigned int value);
	void addULongLong(unsigned long long value);
	void addUChar(unsigned char value);
	void addString(const std::string& value);

protected:
	void addData(const unsigned char* buffer, unsigned int length);

public:
	HashValue m_hash;
};

#endif // HASH_H
