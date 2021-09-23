/*
 WebServe
 Copyright 2019 Peter Pearson.

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

#ifndef SOCKET_LAYER_INTERFACE_H
#define SOCKET_LAYER_INTERFACE_H

#include "configuration.h"
#include "web_server_common.h"

#include "utils/logger.h"

class SocketLayer
{
public:
	SocketLayer(Logger& logger) : m_logger(logger)
	{	
	}
	
	virtual ~SocketLayer()
	{
	}
	
	
	
	// don't think we need this?
	virtual bool isSecureCapable() const
	{
		return false;
	}
	
	virtual bool configure(const Configuration& configuration)
	{
		return true;
	}
	
	virtual bool supportsPerThreadContext() const
	{
		return false;
	}
	
	virtual SocketLayerThreadContext* allocatePerThreadContext()
	{
		return nullptr;
	}
	
	virtual ReturnCodeType allocateSpecialisedConnectionSocket(RequestConnection& connection) = 0;
	
	
protected:
	Logger&		m_logger;
};

#endif // SOCKET_LAYER_INTERFACE_H

