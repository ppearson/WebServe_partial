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

#include "pixel_value_histogram.h"

#include "image/image3f.h"

#include <algorithm>

PixelValueHistogram::PixelValueHistogram()
{

}

void PixelValueHistogram::initWithBitDepthBins(unsigned int bitDepth)
{
	unsigned int numBins = 1 << bitDepth;

	m_counts.resize(numBins, 0);
	m_upperBounds.reserve(numBins);

	float delta = 1.0f / (float)numBins;

	float upperVal = delta;
	for (unsigned int i = 0; i < numBins; i++)
	{
		m_upperBounds.push_back(upperVal);
		upperVal += delta;
	}
}

unsigned int PixelValueHistogram::countPixelValues(const Image3f* pImage)
{
	// TODO: this could be parallelised...
	for (unsigned int y = 0; y < pImage->getHeight(); y++)
	{
		for (unsigned int x = 0; x < pImage->getWidth(); x++)
		{
			// TODO: pointer incr per row to remove indexing?
			const Colour3f& pixel = pImage->getAt(x, y);

			addRawValue(pixel.r);
			addRawValue(pixel.g);
			addRawValue(pixel.b);
		}
	}

	unsigned int numValues = std::count_if(m_counts.begin(), m_counts.end(), [](unsigned int x) { return x > 0; });
	return numValues;
}

void PixelValueHistogram::addRawValue(float value)
{
	auto bound = std::upper_bound(m_upperBounds.begin(), m_upperBounds.end(), value);
	if (bound == m_upperBounds.end())
	{
		// error...
		return;
	}
	unsigned int index = std::distance(m_upperBounds.begin(), bound);
	m_counts[index] += 1;
}
