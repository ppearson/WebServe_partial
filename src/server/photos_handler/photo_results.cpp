/*
 WebServe
 Copyright 2018-2019 Peter Pearson.

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

#include "photo_results.h"

PhotoResults::PhotoResults(const std::vector<PhotoItem>& allPhotos) : //m_aAllPhotos(allPhotos),
	m_haveResults(false),
	m_dateAccessorBuilt(false),
	m_locationAccessorBuilt(false)
{

}

PhotoResults::PhotoResults(const PhotoResults& rhs) : //m_aAllPhotos(rhs.m_aAllPhotos),
	m_haveResults(rhs.m_haveResults),
	m_results(rhs.m_results),
	m_dateAccessorBuilt(rhs.m_dateAccessorBuilt.load()),
	m_locationAccessorBuilt(rhs.m_locationAccessorBuilt.load())
{
	// TODO: what about the actual accessor objects?
}

PhotoResults& PhotoResults::operator=(const PhotoResults& rhs)
{
	m_haveResults = rhs.m_haveResults;
	m_results = rhs.m_results;
	m_dateAccessorBuilt = rhs.m_dateAccessorBuilt.load();
	m_locationAccessorBuilt = rhs.m_locationAccessorBuilt.load();

	// TODO: what about the actual accessor objects?
	
	return *this;
}

void PhotoResults::setResults(const std::vector<const PhotoItem*>& resultItems)
{
	m_results = resultItems;
	m_haveResults = !m_results.empty();
}

void PhotoResults::checkDateAccessorIsValid()
{
	if (m_dateAccessorBuilt.load())
		return;

	// otherwise
	m_dateAccessorBuildLock.lock();

	m_dateAccessor.build(m_results);

	m_dateAccessorBuildLock.unlock();
	m_dateAccessorBuilt = true;
}

void PhotoResults::checkLocationAccessorIsValid()
{
	if (m_locationAccessorBuilt.load())
		return;

	// otherwise
	m_locationAccessorBuildLock.lock();

	m_locationAccessor.build(m_results);

	m_locationAccessorBuildLock.unlock();
	m_locationAccessorBuilt = true;
}
