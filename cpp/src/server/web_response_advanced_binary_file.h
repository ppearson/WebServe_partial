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

#ifndef WEB_RESPONSE_ADVANCED_BINARY_FILE_H
#define WEB_RESPONSE_ADVANCED_BINARY_FILE_H

#include "web_response_advanced.h"

#include <string>

class WebResponseAdvancedBinaryFile : public WebResponseAdvanced
{
public:
	WebResponseAdvancedBinaryFile(const std::string& filePath);

	virtual bool sendResponse(ConnectionSocket* pConnectionSocket, const WebResponseParams& responseParams) const override;
	
	enum ValidationResult
	{
		eOK,
		eFileNotFound,
		eFileTypeNotSupported
	};
	
	ValidationResult validateResponse() const;

protected:
	std::string		m_filePath;
	
	// cached stuff
	std::string		m_contentTypeString;
};

#endif // WEB_RESPONSE_ADVANCED_BINARY_FILE_H
