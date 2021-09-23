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

#include "web_response_advanced_binary_file.h"

#include <sys/stat.h>

#include "utils/socket.h"
#include "utils/file_helpers.h"

#include "web_response.h"

static const unsigned int kMaxSendChunkSize = 1024 * 64; // TODO: decouple read and send?

WebResponseAdvancedBinaryFile::WebResponseAdvancedBinaryFile(const std::string& filePath) :
	WebResponseAdvanced(),
	m_filePath(filePath)
{
	// check the file extension to content type mapping to ensure we support it...
	std::string fileExtension = FileHelpers::getFileExtension(m_filePath);
	
	// TODO: hash this to make it more efficient?

	// images
	if (fileExtension == "jpg" || fileExtension == "jpeg")
	{
		m_contentTypeString = "image/jpeg";
	}
	else if (fileExtension == "png")
	{
		m_contentTypeString = "image/png";
	}
	else if (fileExtension == "gif")
	{
		m_contentTypeString = "image/gif";
	}
	else if (fileExtension == "svg")
	{
		m_contentTypeString = "image/svg+xml";
	}
	else if (fileExtension == "bmp")
	{
		m_contentTypeString = "image/bmp";
	}
	// text
	else if (fileExtension == "html" || fileExtension == "htm")
	{
		m_contentTypeString = "text/html";
	}
	else if (fileExtension == "txt")
	{
		m_contentTypeString = "text/txt";
	}
	else if (fileExtension == "css")
	{
		m_contentTypeString = "text/css";
	}
	else if (fileExtension == "js")
	{
		m_contentTypeString = "application/javascript";
	}
	else if (fileExtension == "json")
	{
		m_contentTypeString = "application/json";
	}
	// other
	else if (fileExtension == "mp3")
	{
		m_contentTypeString = "audio/mpeg3";
	}
	else if (fileExtension == "pdf")
	{
		m_contentTypeString = "application/pdf";
	}
	else if (fileExtension == "zip")
	{
		m_contentTypeString = "application/zip";
	}
	else if (fileExtension == "bz2")
	{
		m_contentTypeString = "application/x-bzip2";
	}
	else if (fileExtension == "tgz")
	{
		m_contentTypeString = "application/x-compressed";
	}
}

bool WebResponseAdvancedBinaryFile::sendResponse(ConnectionSocket* pConnectionSocket, const WebResponseParams& responseParams) const
{
	// TODO: don't print full path in any error
	
	if (m_contentTypeString.empty())
	{
		return false;
	}

	std::string responseString;
	char szTemp[64];
	memset(szTemp, 0, 64);

	int returnCode = 200;
	sprintf(szTemp, "HTTP/1.1 %i\r\n", returnCode);
	responseString += szTemp;

	WebResponseCommon::addCommonResponseHeaderItems(responseString, responseParams);

	responseString += "Content-Type: " + m_contentTypeString + "\r\n";

	if (responseParams.useChunkedLargeFiles)
	{
		// this is the last header string, so we need to end the header...
		responseString += "Transfer-Encoding: chunked\r\n\r\n";
	}
	else
	{
		// attempt to disable Safari assuming chunked encoding, although it seems to ignore this...
//		responseString += "Content-Encoding: identity\r\n";

		// It's not 100% clear from RFC 2616 if this is part of the spec or not... 14.41 doesn't mention it, but
		// section 4.4 does mention it with regards to message length...
//		responseString += "Transfer-Encoding: identity\r\n";
	}

	// work out the data length
	// TODO: move this somewhere more re-usable
	struct stat statBuff;
	if (stat(m_filePath.c_str(), &statBuff) == -1)
	{
		return false;
	}

	size_t fileSizeInBytes = statBuff.st_size;

	if (!responseParams.useChunkedLargeFiles)
	{
		// only need to the content-length if we're not chunked...

		memset(szTemp, 0, 64);
		sprintf(szTemp, "Content-Length: %zu\r\n\r\n", fileSizeInBytes);
		responseString += szTemp;
	}

	// send this header info
	pConnectionSocket->send(responseString, 0);

	// now send the raw content
	// TODO: this should really be above, and it's unlikely to fail if stat worked...
	FILE* pDataFile = fopen(m_filePath.c_str(), "rb");
	if (!pDataFile)
	{
		return false;
	}

	size_t bytesToRead = fileSizeInBytes;

	bool wasSuccessful = true;

	if (responseParams.useChunkedLargeFiles)
	{
		// if we're using chunked, we need extra memory allocated for the chunk size + \r\n at the beginning and the \r\n at the end of the
		// chunk data block.

		// work out the size of the initial chunk size part...
		sprintf(szTemp, "%02X\r\n", kMaxSendChunkSize);

		unsigned int maxChunkHeaderLength = strlen(szTemp);

		unsigned int bufferSize = maxChunkHeaderLength + kMaxSendChunkSize + 2;

		unsigned char* pDataBuffer = new unsigned char[bufferSize];

		while (bytesToRead > 0)
		{
			unsigned char* pDataBufferNextPos = pDataBuffer;
			size_t thisChunkDataSize = (bytesToRead >= kMaxSendChunkSize) ? kMaxSendChunkSize : bytesToRead;

			// write chunk size header to start of data buffer
			sprintf(szTemp, "%02X\r\n", (unsigned int)thisChunkDataSize);
			unsigned int chunkHeaderLength = strlen(szTemp);
			memcpy(pDataBufferNextPos, szTemp, chunkHeaderLength);

			unsigned int totalChunkMessageSize = chunkHeaderLength + thisChunkDataSize + 2;

			pDataBufferNextPos += chunkHeaderLength;

			size_t dataRead = fread(pDataBufferNextPos, 1, thisChunkDataSize, pDataFile);
			if (dataRead != thisChunkDataSize)
			{
				wasSuccessful = false;
				break;
			}

			pDataBufferNextPos += dataRead;

			memcpy(pDataBufferNextPos, "\r\n", 2);

			if (!pConnectionSocket->send(pDataBuffer, totalChunkMessageSize))
			{
				wasSuccessful = false;
				break;
			}

			bytesToRead -= thisChunkDataSize;
		}

		delete [] pDataBuffer;

		if (wasSuccessful)
		{
			// now send a final 0\r\n as the last chunk...
			pConnectionSocket->send("0\r\n");
		}
	}
	else
	{
		unsigned char* pDataBuffer = new unsigned char[kMaxSendChunkSize];

		while (bytesToRead > 0)
		{
			size_t thisChunkSize = (bytesToRead >= kMaxSendChunkSize) ? kMaxSendChunkSize : bytesToRead;

			size_t dataRead = fread(pDataBuffer, 1, thisChunkSize, pDataFile);
			if (dataRead != thisChunkSize)
			{
				wasSuccessful = false;
				break;
			}

			if (!pConnectionSocket->send(pDataBuffer, thisChunkSize))
			{
				wasSuccessful = false;
				break;
			}

			bytesToRead -= thisChunkSize;
		}

		delete [] pDataBuffer;
	}

	fclose(pDataFile);

	if (!wasSuccessful)
	{
		// on the assumption that the socket was closed by the other side...
		// TODO: this needs to be made more robust to correctly check that that was the case...
		return false;
	}

	if (responseParams.useChunkedLargeFiles)
	{
		// now send final "\r\n"
		pConnectionSocket->send("\r\n");
	}

	// caller will handle (possibly) closing socket

	return true;
}

WebResponseAdvancedBinaryFile::ValidationResult WebResponseAdvancedBinaryFile::validateResponse() const
{
	struct stat statBuff;
	if (stat(m_filePath.c_str(), &statBuff) == -1)
	{
		return eFileNotFound;
	}
	
	if (m_contentTypeString.empty())
	{
		return eFileTypeNotSupported;
	}
	
	return eOK;
}
