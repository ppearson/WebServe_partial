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

#ifndef IMAGE3F_H
#define IMAGE3F_H

#include <vector>
#include <algorithm>
#include <cmath>

struct Colour3f
{
	Colour3f() : r(0.0f), g(0.0f), b(0.0f)
	{

	}

	Colour3f(float rV, float gV, float bV) : r(rV), g(gV), b(bV)
	{

	}

	Colour3f operator*(float scale) const
	{
		return Colour3f(scale * r, scale * g, scale * b);
	}

	Colour3f operator/(float scale) const
	{
		float inv = 1.0f / scale;
		return Colour3f(r * inv, g * inv, b * inv);
	}

	Colour3f operator/(const Colour3f& rhs) const
	{
		return Colour3f(r / rhs.r, g / rhs.g, b / rhs.b);
	}

	Colour3f operator-(const Colour3f& rhs) const
	{
		return Colour3f(r - rhs.r, g - rhs.g, b - rhs.b);
	}

	Colour3f operator+(const Colour3f& rhs) const
	{
		return Colour3f(r + rhs.r, g + rhs.g, b + rhs.b);
	}

	Colour3f operator*(const Colour3f& rhs) const
	{
		return Colour3f(r * rhs.r, g * rhs.g, b * rhs.b);
	}

	Colour3f& operator*=(float scale)
	{
		r *= scale;
		g *= scale;
		b *= scale;

		return *this;
	}

	Colour3f& operator+=(const Colour3f& rhs)
	{
		r += rhs.r;
		g += rhs.g;
		b += rhs.b;

		return *this;
	}

	Colour3f& operator*=(const Colour3f& rhs)
	{
		r *= rhs.r;
		g *= rhs.g;
		b *= rhs.b;

		return *this;
	}

	Colour3f& operator/=(const Colour3f& rhs)
	{
		r /= rhs.r;
		g /= rhs.g;
		b /= rhs.b;

		return *this;
	}

	void clamp()
	{
		r = std::min(std::max(r, 0.0f), 1.0f);
		g = std::min(std::max(g, 0.0f), 1.0f);
		b = std::min(std::max(b, 0.0f), 1.0f);
	}

	void abs()
	{
		r = std::abs(r);
		g = std::abs(g);
		b = std::abs(b);
	}

	float getMaxVal() const
	{
		return std::max(std::max(r, g), b);
	}

	float average() const
	{
		return (r + g + b) * 0.3333333f;
	}

	float brightness() const
	{
		return (0.2126f * r) + (0.7152f * g) + (0.0722f * b);
	}

	float		r;
	float		g;
	float		b;
};

class Image3f
{
public:
	Image3f();
	Image3f(unsigned int width, unsigned int height);

	void setSize(unsigned int width, unsigned int height);

	const Colour3f* getRowPtr(unsigned int row) const;
	Colour3f* getRowPtr(unsigned int row);

	// these are unclamped
	const Colour3f& getAt(unsigned int x, unsigned int y) const;
	Colour3f& getAt(unsigned int x, unsigned int y);

	// these clamp between 0 and res - 1
	const Colour3f& getAtClamped(unsigned int x, unsigned int y) const;
	Colour3f& getAtClamped(unsigned int x, unsigned int y);


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

	std::vector<Colour3f>		m_aPixelData;
};

#endif // IMAGE3F_H
