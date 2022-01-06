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

#ifndef PHOTO_REPRESENTATIONS_H
#define PHOTO_REPRESENTATIONS_H

#include <string>
#include <vector>

class PhotoRepresentations
{
public:
	PhotoRepresentations();

	class PhotoRep
	{
	public:
		PhotoRep() : m_width(0), m_height(0)
		{
		}

		PhotoRep(const std::string& relativePath, uint16_t width, uint16_t height) : m_relativeFilePath(relativePath),
				m_width(width), m_height(height)
		{
			m_aspectRatio = float(m_width) / (float)m_height;
		}

		const std::string& getRelativeFilePath() const
		{
			return m_relativeFilePath;
		}

		uint16_t getWidth() const
		{
			return m_width;
		}

		uint16_t getHeight() const
		{
			return m_height;
		}

		float getAspectRatio() const
		{
			return m_aspectRatio;
		}

	private:
		// currently relative file path from photosBasePath
		// TODO: reduce duplicated file path strings...
		// Note:  currently, this is optimised for performance (in terms of not having to concat different filename parts on-the-fly)
		//        as opposed to memory usage.
		std::string			m_relativeFilePath;

		uint16_t			m_width;
		uint16_t			m_height;

		float				m_aspectRatio;
	};


	void addRepresentation(const PhotoRep& photoRepr);

	const PhotoRep* getFirstRepresentationMatchingCriteriaMaxDimension(unsigned int maxVal, bool returnSmallestIfNotFound) const;
	const PhotoRep* getFirstRepresentationMatchingCriteriaMinDimension(unsigned int minVal, bool returnLargestIfNotFound) const;

	const PhotoRep* getLargestRepresentationMatchingCriteriaMaxDimension(unsigned int maxVal) const;
	const PhotoRep* getSmallestRepresentationMatchingCriteriaMinDimension(unsigned int minVal) const;

protected:
	std::vector<PhotoRep>		m_aRepresentations;
};

#endif // PHOTO_REPRESENTATIONS_H
