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

#ifndef PHOTO_RESULTS_H
#define PHOTO_RESULTS_H

#include <vector>
#include <memory>
#include <atomic>
#include <condition_variable>

#include "photo_item.h"
#include "photo_results_date_accessor.h"
#include "photo_results_location_accessor.h"

class PhotoResults
{
public:
	PhotoResults(const std::vector<PhotoItem>& allPhotos);

	PhotoResults(const PhotoResults& rhs);
	
	PhotoResults& operator=(const PhotoResults& rhs);

	void setResults(const std::vector<const PhotoItem*>& resultItems);

	const std::vector<const PhotoItem*>& getAllResults() const
	{
		return m_results;
	}

	void checkDateAccessorIsValid();

	const PhotoResultsDateAccessor& getDateAccessor() const
	{
		return m_dateAccessor;
	}

	void checkLocationAccessorIsValid();

	const PhotoResultsLocationAccessor& getLocationAccessor() const
	{
		return m_locationAccessor;
	}

protected:
	// reference pointing to all items in the PhotoCatalogue.
	// for the moment, this isn't used, but in the future it will be...
//	const std::vector<PhotoItem>&	m_aAllPhotos;

	bool							m_haveResults;
	std::vector<const PhotoItem*>	m_results;

	std::atomic<bool>				m_dateAccessorBuilt;
	std::mutex						m_dateAccessorBuildLock;
	PhotoResultsDateAccessor		m_dateAccessor;

	std::atomic<bool>				m_locationAccessorBuilt;
	std::mutex						m_locationAccessorBuildLock;
	PhotoResultsLocationAccessor	m_locationAccessor;
};

typedef std::shared_ptr<const PhotoResults> PhotoResultsPtr;

#endif // PHOTO_RESULTS_H
