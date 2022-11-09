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

#include "image3f.h"

#include <memory.h>

Image3f::Image3f()
{

}

Image3f::Image3f(unsigned int width, unsigned int height) : m_width(width),
	m_height(height)
{
	m_aPixelData.resize(width * height);
}

void Image3f::setSize(unsigned int width, unsigned int height)
{
	m_aPixelData.resize(width * height);
	m_width = width;
	m_height = height;
}

const Colour3f* Image3f::getRowPtr(unsigned int row) const
{
	unsigned int index = row * m_width;
	return &m_aPixelData[index];
}

Colour3f* Image3f::getRowPtr(unsigned int row)
{
	unsigned int index = row * m_width;
	return &m_aPixelData[index];
}

const Colour3f& Image3f::getAt(unsigned int x, unsigned int y) const
{
	unsigned int index = (y * m_width) + x;
	return m_aPixelData[index];
}

Colour3f& Image3f::getAt(unsigned int x, unsigned int y)
{
	unsigned int index = (y * m_width) + x;
	return m_aPixelData[index];
}

const Colour3f& Image3f::getAtClamped(unsigned int x, unsigned int y) const
{
	x = std::max(std::min(x, m_width - 1), 0u);
	y = std::max(std::min(y, m_height - 1), 0u);

	unsigned int index = (y * m_width) + x;
	return m_aPixelData[index];
}

Colour3f& Image3f::getAtClamped(unsigned int x, unsigned int y)
{
	x = std::max(std::min(x, m_width - 1), 0u);
	y = std::max(std::min(y, m_height - 1), 0u);

	unsigned int index = (y * m_width) + x;
	return m_aPixelData[index];
}

void Image3f::flipImageVertically()
{
	Colour3f* pTempScanline = new Colour3f[m_width];

	unsigned int scanlines = m_height;

	unsigned int scanlineBytes = m_width * sizeof(Colour3f);

	// only need to do half of them, as we're swapping top and bottom
	unsigned int numRows = scanlines / 2;

	unsigned int targetScanline = scanlines - 1;

	for (unsigned int i = 0; i < numRows; i++)
	{
		Colour3f* pSrc = getRowPtr(i);

		memcpy(pTempScanline, pSrc, scanlineBytes);

		Colour3f* pDst = getRowPtr(targetScanline);

		// copy bottom to top
		memcpy(pSrc, pDst, scanlineBytes);

		// copy temp to bottom
		memcpy(pDst, pTempScanline, scanlineBytes);

		targetScanline --;
	}

	if (pTempScanline)
	{
		delete [] pTempScanline;
		pTempScanline = nullptr;
	}
}
