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

#ifndef IMAGE1F_H
#define IMAGE1F_H

#include <vector>
#include <algorithm>
#include <cmath>

class Image1f
{
public:
	Image1f();
	Image1f(unsigned int width, unsigned int height);

	void setSize(unsigned int width, unsigned int height);

	const float* getRowPtr(unsigned int row) const;
	float* getRowPtr(unsigned int row);

	// these are unclamped
	const float& getAt(unsigned int x, unsigned int y) const;
	float& getAt(unsigned int x, unsigned int y);

	// these clamp between 0 and res - 1
	const float& getAtClamped(unsigned int x, unsigned int y) const;
	float& getAtClamped(unsigned int x, unsigned int y);


	unsigned int getWidth() const
	{
		return m_width;
	}

	unsigned int getHeight() const
	{
		return m_height;
	}

protected:
	unsigned int		m_width	= 0;
	unsigned int		m_height = 0;

	std::vector<float>		m_aPixelData;
};

#endif // IMAGE1F_H
