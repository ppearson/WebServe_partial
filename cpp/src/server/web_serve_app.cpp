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

#include <cstdio>
#include <csignal>

#include "web_server_service.h"
#include "main_request_handler.h"

#include "configuration.h"

#define ENABLE_SIGNAL_HANDLING 1

#if ENABLE_SIGNAL_HANDLING
static WebServerService* gWebService = nullptr;

void sigintHandler(int signalVal)
{
	if (gWebService)
	{
		gWebService->stop();
	}
}
#endif

void printHelp()
{
	fprintf(stderr, "webserve\n\n");
	fprintf(stderr, "webserve\n");
	fprintf(stderr, "webserve --config [config_path]\n");
}

int main(int argc, char** argv)
{
	Configuration config;

	if (argc == 2 && strcmp(argv[1], "--help") == 0)
	{
		printHelp();
		return 0;
	}

	bool haveLoadedConfig = false;

	if (argc == 3)
	{
		if (strcmp(argv[1], "--config") == 0)
		{
			std::string configPath = argv[2];
			if (!config.loadFromFile(configPath))
			{
				fprintf(stderr, "Error loading config file: %s\n", configPath.c_str());
				return -1;
			}
			else
			{
				haveLoadedConfig = true;
			}
		}
	}

	if (!haveLoadedConfig)
	{
		config.autoLoadFile();
	}

	WebServerService web;
	if (!web.configure(config))
	{
		return -1;
	}
	
	// Note: this bit is done first to separate the binding and optional downgrading of username
	//       such that any request handler stuff (i.e. AccessControl) can create logs with the
	//       downgraded username instead of as root
	if (!web.bindSocketsAndPrepare())
	{
		return -1;
	}
	
#if ENABLE_SIGNAL_HANDLING
	gWebService = &web;
	
	signal(SIGINT, sigintHandler);
	signal(SIGTERM, sigintHandler);
#endif

	MainRequestHandler requestHandler;
	// TODO: this logger thing is messy...
	requestHandler.configure(config, web.getLogger());

	web.setRequestHandler(&requestHandler, false);

	web.start();
	
	fprintf(stderr, "WebServe main() returned.\n");

	return 0;
}
