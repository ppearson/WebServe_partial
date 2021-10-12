/*
 WebServe (Rust port)
 Copyright 2021 Peter Pearson.

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

mod configuration;
mod thread_pool;
mod file_helpers;
mod main_request_handler;
mod file_request_handler;
mod photos_handler;
mod sub_request_handler;
mod string_helpers;
mod web_request;
mod web_response_advanced;
mod web_response_generator;
mod web_server_common;
mod web_server_service;
mod uri_helpers;

use web_server_service::WebServerService;

fn main() {
    let mut config = configuration::Configuration::new();
    config.auto_load_file();

    let web_server_service = WebServerService::init(&config);

    web_server_service.run();
}
