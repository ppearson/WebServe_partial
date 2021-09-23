/*
 WebServe
 Copyright 2018 Peter Pearson.

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

#include "photo_representations.h"

PhotoRepresentations::PhotoRepresentations()
{

}

void PhotoRepresentations::addRepresentation(const PhotoRep& photoRepr)
{
	m_aRepresentations.emplace_back(photoRepr);
}

const PhotoRepresentations::PhotoRep* PhotoRepresentations::getFirstRepresentationMatchingCriteriaMaxDimension(unsigned int maxVal, bool returnSmallestIfNotFound) const
{
	// for the moment, do a brute-force search..
	// TODO: in the future we could try and order the representations size-wise and do a binary search, but given we have to satisfy both dimensions
	//       that might not be trivial in terms of ordering...

	unsigned int smallestValFound = 6000;
	unsigned int smallestIndex = -1;

	unsigned int index = 0;
	for (const PhotoRep& repr : m_aRepresentations)
	{
		unsigned int maxDim = std::max(repr.getWidth(), repr.getHeight());
		if (maxDim <= maxVal)
		{
			return &repr;
		}

		if (maxDim < smallestValFound)
		{
			smallestValFound = maxDim;
			smallestIndex = index++;
		}
	}

	if (!returnSmallestIfNotFound)
	{
		// we didn't find anything matching that criteria...
		return nullptr;
	}

	// otherwise, return the smallest we previously found...
	if (smallestIndex != -1)
	{
		const PhotoRepresentations::PhotoRep* pSmallestBackup = &m_aRepresentations[smallestIndex];
		return pSmallestBackup;
	}

	return nullptr;
}

const PhotoRepresentations::PhotoRep* PhotoRepresentations::getFirstRepresentationMatchingCriteriaMinDimension(unsigned int minVal, bool returnLargestIfNotFound) const
{
	// for the moment, do a brute-force search..
	// TODO: in the future we could try and order the representations size-wise and do a binary search, but given we have to satisfy both dimensions
	//       that might not be trivial in terms of ordering...

	unsigned int largestValFound = 1;
	unsigned int largestIndex = -1;

	unsigned int index = 0;
	for (const PhotoRep& repr : m_aRepresentations)
	{
		unsigned int minDim = std::min(repr.getWidth(), repr.getHeight());
		if (minDim >= minVal)
		{
			return &repr;
		}

		if (minDim > largestValFound)
		{
			largestValFound = minDim;
			largestIndex = index++;
		}
	}

	if (!returnLargestIfNotFound)
	{
		// we didn't find anything matching that criteria...
		return nullptr;
	}

	// otherwise, return the largest we previously found...
	if (largestIndex != -1)
	{
		const PhotoRepresentations::PhotoRep* pLargestBackup = &m_aRepresentations[largestIndex];
		return pLargestBackup;
	}

	return nullptr;
}

const PhotoRepresentations::PhotoRep* PhotoRepresentations::getLargestRepresentationMatchingCriteriaMaxDimension(unsigned int maxVal) const
{
	unsigned int largestValFound = 1;
	unsigned int largestIndex = -1;

	unsigned int index = 0;
	for (const PhotoRep& repr : m_aRepresentations)
	{
		unsigned int maxDim = std::max(repr.getWidth(), repr.getHeight());

		if (maxDim <= maxVal && maxDim > largestValFound)
		{
			largestValFound = maxDim;
			largestIndex = index++;
		}
		else
		{
			index++;
		}
	}

	// return the largest we found...
	if (largestIndex != -1)
	{
		const PhotoRepresentations::PhotoRep* pLargestBackup = &m_aRepresentations[largestIndex];
		return pLargestBackup;
	}

	return nullptr;
}

const PhotoRepresentations::PhotoRep* PhotoRepresentations::getSmallestRepresentationMatchingCriteriaMinDimension(unsigned int minVal) const
{
	unsigned int smallestValFound = 12000;
	unsigned int smallestIndex = -1;

	unsigned int index = 0;
	for (const PhotoRep& repr : m_aRepresentations)
	{
		unsigned int maxDim = std::max(repr.getWidth(), repr.getHeight());

		if (maxDim >= minVal && maxDim < smallestValFound)
		{
			smallestValFound = maxDim;
			smallestIndex = index++;
		}
		else
		{
			index++;
		}
	}

	// return the smallest we found...
	if (smallestIndex != -1)
	{
		const PhotoRepresentations::PhotoRep* pSmallestBackup = &m_aRepresentations[smallestIndex];
		return pSmallestBackup;
	}

	return nullptr;
}
