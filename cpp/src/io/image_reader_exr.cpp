/*
 WebServe
 Copyright 2018-2022 Peter Pearson.
 Originally taken from:
 Imagine
 Copyright 2011-2014 Peter Pearson.

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

#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfTestFile.h> // for isOpenExrFile
#include <ImfPixelType.h>

#include <IexBaseExc.h>

#include <half.h>

#include "image_reader_exr.h"

ImageReaderEXR::ImageReaderEXR()
{

}

bool ImageReaderEXR::readMetadata(const std::string& filePath, EXRInfo& info) const
{
	if (!Imf::isOpenExrFile(filePath.c_str(), info.tiled))
	{
		return false;
	}

	Imf::InputFile inputFile(filePath.c_str());

	Imath::Box2i imageBounds = inputFile.header().dataWindow();
	info.width = imageBounds.max.x - imageBounds.min.x + 1;
	info.height = imageBounds.max.y - imageBounds.min.y + 1;

	info.increasingY = inputFile.header().lineOrder() == Imf::INCREASING_Y;

	const Imf::ChannelList& channelList = inputFile.header().channels();
	Imf::ChannelList::ConstIterator chanIt = channelList.begin();
	unsigned int channelCount = 0;
	for (; chanIt != channelList.end(); ++chanIt)
	{
		std::string channelName = chanIt.name();
		EXRInfo::EXRChannel newChannel(channelName);

		newChannel.dataType = EXRInfo::eFloat;
		if (chanIt.channel().type == Imf::HALF)
			newChannel.dataType = EXRInfo::eHalf;
		else if (chanIt.channel().type == Imf::UINT)
			newChannel.dataType = EXRInfo::eUInt;

		if (channelName == "A")
			info.channelFlags |= EXRInfo::eAlpha;
		else if (channelName == "R")
			info.channelFlags |= EXRInfo::eRed;
		else if (channelName == "G")
			info.channelFlags |= EXRInfo::eGreen;
		else if (channelName == "B")
			info.channelFlags |= EXRInfo::eBlue;

		newChannel.index = channelCount;

		info.aChannels.emplace_back(newChannel);

		channelCount ++;
	}

	return true;
}

bool ImageReaderEXR::getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const
{
	EXRInfo info;
	if (!readMetadata(filePath, info))
	{
		return false;
	}

	imageDetails.width = info.width;
	imageDetails.height = info.height;

	imageDetails.channels = info.aChannels.size();
	// each channel can be different, but probably good enough to just use first channel for our purposes for RGB images.
	if (imageDetails.channels > 0)
	{
		if (info.aChannels[0].dataType == EXRInfo::eFloat)
		{
			imageDetails.floatingPointData = true;
			imageDetails.pixelBitDepth = 32;
		}
		else if (info.aChannels[0].dataType == EXRInfo::eHalf)
		{
			imageDetails.floatingPointData = true;
			imageDetails.pixelBitDepth = 16;
		}
		else
		{
			// uint32_t, but it's very unlikely we're going to want to do anything with that...
		}
	}

	return true;
}

bool ImageReaderEXR::extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const
{
	return false;
}

Image3f* ImageReaderEXR::readColour3fImage(const std::string& filePath) const
{
	EXRInfo info;
	if (!readMetadata(filePath, info))
		return nullptr;

	unsigned int numThreads = Imf::globalThreadCount();

	if (info.aChannels.size() < 3)
	{
		fprintf(stderr, "Error: Incorrect number of channels found in EXR file: %s\n", filePath.c_str());
		return nullptr;
	}

	Imf::InputFile inputFile(filePath.c_str(), numThreads);

	Image3f* pImage3f = new Image3f(info.width, info.height);
	// TODO: this is an assuption that all channels will be the same, which we can't guarentee, but is likely for our purposes of RGB images...
	char* pData = (char*)pImage3f->getRowPtr(0);

	Imf::FrameBuffer frameBuffer;
	// we always want to have float32
	Imf::PixelType pixelType = Imf::FLOAT;

	size_t pixelBytes = 1;
	pixelBytes *= sizeof(float);

	unsigned int channelsToRead = 3;

	size_t scanlineBytes = pixelBytes * info.width * channelsToRead;

	if (info.hasChannelFlags(EXRInfo::eRed | EXRInfo::eGreen | EXRInfo::eBlue))
	{
		frameBuffer.insert("R", Imf::Slice(pixelType, pData, pixelBytes * channelsToRead, scanlineBytes));
		frameBuffer.insert("G", Imf::Slice(pixelType, pData + pixelBytes, pixelBytes * channelsToRead, scanlineBytes));
		frameBuffer.insert("B", Imf::Slice(pixelType, pData + pixelBytes + pixelBytes, pixelBytes * channelsToRead, scanlineBytes));
	}
	else
	{
		// otherwise, for the moment, just pick the first three...
		frameBuffer.insert(info.aChannels[0].name, Imf::Slice(pixelType, pData, pixelBytes * channelsToRead, scanlineBytes));
		frameBuffer.insert(info.aChannels[1].name, Imf::Slice(pixelType, pData + pixelBytes, pixelBytes * channelsToRead, scanlineBytes));
		frameBuffer.insert(info.aChannels[2].name, Imf::Slice(pixelType, pData + pixelBytes + pixelBytes, pixelBytes * channelsToRead, scanlineBytes));
	}

	inputFile.setFrameBuffer(frameBuffer);

	try
	{
		inputFile.readPixels(0, info.height - 1);
	}
	catch (const Iex::BaseExc& e)
	{
		fprintf(stderr, "Error reading EXR: %s - %s\n", filePath.c_str(), e.what());
		return nullptr;
	}

	// if order was increasing, we need to flip the image - do this scanline-by-scanline so we
	// don't have to allocate any more memory (but we thrash the cache a bit more)...
	if (info.increasingY)
	{
		pImage3f->flipImageVertically();
	}

	return pImage3f;
}


namespace
{
	ImageReader* createImageReaderEXR()
	{
		return new ImageReaderEXR();
	}

	const bool registered = FileIORegistry::instance().registerImageReader("exr", createImageReaderEXR);
}
