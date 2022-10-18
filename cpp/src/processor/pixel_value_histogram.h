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

#ifndef PIXEL_VALUE_HISTOGRAM_H
#define PIXEL_VALUE_HISTOGRAM_H

#include <vector>

class Image3f;

class PixelValueHistogram
{
public:
	PixelValueHistogram();

	void initWithBitDepthBins(unsigned int bitDepth);

	unsigned int countPixelValues(const Image3f* pImage);


private:
	void addRawValue(float value);

private:

	// bins
	std::vector<float>			m_upperBounds;
	std::vector<unsigned int>	m_counts;
};

#endif // PIXEL_VALUE_HISTOGRAM_H
