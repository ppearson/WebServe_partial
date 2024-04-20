/*
 WebServe (Rust port)
 Copyright 2021-2024 Peter Pearson.

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

use crate::configuration::Configuration;
use crate::sub_request_handler::{SubRequestHandler, HandleRequestResult};
use crate::file_request_handler::FileRequestHandler;
use crate::photos_handler::photos_request_handler::PhotosRequestHandler;
use crate::web_request::{ConnectionType, WebRequest};
use crate::web_response_generator::{GetResponseString, WebResponseGeneratorText};
use crate::web_server_common::RequestConnection;
use crate::uri_helpers::*;

use std::io::prelude::*;

use std::collections::HashMap;

use std::time::Duration;

pub struct MainRequestHandler {
    sub_request_handlers:       Vec<Box<dyn SubRequestHandler + Send + Sync> >,

    // for the moment, a complete clone...
    configuration:                  Configuration,

    // below point into above Vec
    have_dir_sub_request_handlers:  bool,
    dir_handler_lookups:            HashMap<String, usize>,

    have_host_sub_request_handlers: bool,
    host_handler_lookups:           HashMap<String, usize>,

    fallback_handler:               Option<usize>,
}

impl MainRequestHandler {

    pub fn init(config: &Configuration) -> MainRequestHandler {
        let mut mrh = MainRequestHandler{ sub_request_handlers: Vec::with_capacity(0),
                                          configuration: config.clone(),
                                          have_dir_sub_request_handlers: false, dir_handler_lookups: HashMap::new(),
                                          have_host_sub_request_handlers: false, host_handler_lookups: HashMap::new(),
                                          fallback_handler: None };
        mrh.configure_sub_request_handlers(config);

        mrh
    }

    fn configure_sub_request_handlers(&mut self, config: &Configuration) {
        let mut count : usize = 0;
        for site_config in &config.site_configs {
            if site_config.ctype == "files" {
                let mut new_handler = FileRequestHandler::new();
                new_handler.configure(site_config, config);
                self.sub_request_handlers.push(Box::new(new_handler));
            }
            else if site_config.ctype == "photos" {
                let mut new_handler = PhotosRequestHandler::new();
                new_handler.configure(site_config, config);
                self.sub_request_handlers.push(Box::new(new_handler));
            }
            else {
                eprintln!("Error: Unhandled site config type...");
                continue;
            }

            if let Some((def_type, def_value)) = site_config.definition.split_once(':') {
                if def_type == "dir" {
                    self.have_dir_sub_request_handlers = true;
                    self.dir_handler_lookups.insert(def_value.to_string(), count);
                }
                else if def_type == "host" {
                    self.have_host_sub_request_handlers = true;
                    self.host_handler_lookups.insert(def_value.to_string(), count);
                }
                else if def_type == "*" {
                    // it's a wildcard fallback
                    if self.fallback_handler.is_some() {
                        eprintln!("Error: A fallback handler exists already.");
                        continue;
                    }
                    self.fallback_handler = Some(count);
                }
                else {
                    eprintln!("Invalid site config definition type: '{}'", def_type);
                }
            }
            else if site_config.definition == "*" {
                // allow a single fallback type without a config value (i.e. no ':' char in the string)
                if self.fallback_handler.is_some() {
                    eprintln!("Error: A fallback handler exists already.");
                    continue;
                }
                self.fallback_handler = Some(count);
            }

            count += 1;
        }
    }

    pub fn handle_request(&self, connection: RequestConnection) {
        let mut buffer = [0; 2048];
    
        let mut stream = &connection.tpc_stream;
        let read_res = stream.read(&mut buffer);
        if read_res.is_err() {
            eprintln!("Error reading tpc_stream: {}", read_res.unwrap_err());
            return;
        }

        let mut connection_keep_alive_count = 0u32;
        let mut should_keep_alive_next_time = self.configuration.keep_alive_enabled;

        let mut request_string = String::from_utf8_lossy(&buffer).to_string();

        loop {
    
            let web_request = WebRequest::create_from_string(&request_string);
            if web_request.is_err() {
                println!("Invalid request received. Ignoring and aborting connection...");
                return;
            }
        
            let web_request = web_request.unwrap();

            if web_request.path.is_empty() {
                println!("Empty path request. Ignoring...");
                return;
            }

            should_keep_alive_next_time = should_keep_alive_next_time && web_request.connection_type == ConnectionType::ConnectionKeepAlive;

            // knock the leading slash off so everything' relative to our root...
            let request_path = web_request.path[1..].to_string();

            let mut handle_request_result = HandleRequestResult::RequestNotHandled;

            // try hosts first...
            if self.have_host_sub_request_handlers {
                if let Some(sub_handler_index) = self.host_handler_lookups.get(&web_request.host_value) {
                    let sub_handler = &self.sub_request_handlers[*sub_handler_index];

                    // TODO: return value...
                    handle_request_result = sub_handler.handle_request(&connection, &web_request, &request_path);
                }
            }
            else if self.have_dir_sub_request_handlers {
                // try directory handlers
                let mut first_level_dir = String::new();
                let mut remainder_uri = String::new();
                if split_first_level_directory_and_remainder(&request_path, &mut first_level_dir, &mut remainder_uri) {
                    // do nothing
                }
                else {
                    // maybe there wasn't a trailing slash...
                    first_level_dir = request_path.clone();
                }

                if let Some(sub_handler_index) = self.dir_handler_lookups.get(&first_level_dir) {
                    let sub_handler = &self.sub_request_handlers[*sub_handler_index];
                    // TODO: return value...
                    handle_request_result = sub_handler.handle_request(&connection, &web_request, &remainder_uri);
                }
            }

            if !handle_request_result.was_handled() && self.fallback_handler.is_some() {
                let sub_handler = &self.sub_request_handlers[self.fallback_handler.unwrap()];
                handle_request_result = sub_handler.handle_request(&connection, &web_request, &request_path);
            }

            if handle_request_result.was_error() {
                // it was an error, most likely the socket was closed by the client, so just abort.
                return;
            }

            if !handle_request_result.was_handled() {
                if request_path != "favicon.ico" && !request_path.is_empty() {
                    println!("Unknown handler for: '{}'", request_path);
                }
    
                let wrg = WebResponseGeneratorText::new(404, "Not found.");
    
                let response = wrg.get_response_string();
            
                let write_res = stream.write(response.as_bytes());
                if write_res.is_err() {
                    return;
                }

                stream.flush().unwrap();
            }

            should_keep_alive_next_time = should_keep_alive_next_time && connection_keep_alive_count < self.configuration.keep_alive_limit;
            if should_keep_alive_next_time {
                connection_keep_alive_count += 1;

                // set timeout on socket
                let to_res = stream.set_read_timeout(Some(Duration::new(self.configuration.keep_alive_timeout as u64, 0)));
                if to_res.is_err() {
                    eprintln!("Error setting socket timeout for keep alive...");
                    return;
                }

                let read_res = stream.read(&mut buffer);
                if read_res.is_err() {
                    let error = read_res.unwrap_err();
                    let kind = error.kind();
                    if kind == std::io::ErrorKind::WouldBlock {
                        // ignore, as it's likely a timeout...
                    }
                    else if kind == std::io::ErrorKind::ConnectionReset || kind == std::io::ErrorKind::ConnectionAborted {

                    }
                    else {
                        eprintln!("Error reading tpc_stream for keep alive: {}, kind: {:?}", error, kind);
                    }
                    return;
                }

                request_string = String::from_utf8_lossy(&buffer).to_string();

//                println!("Re-used connection: {} times...", connection_keep_alive_count);
            }
            else {
                // break out...
                break;
            }
        }
    }
}
