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

use crate::configuration::{Configuration, SiteConfig};
use crate::sub_request_handler::{SubRequestHandler, HandleRequestResult};
use crate::web_request::WebRequest;
use crate::web_response_generator::{GetResponseString, WebResponseGeneratorFile};
use crate::web_server_common::RequestConnection;

use std::io::prelude::*;
use std::path::Path;

pub struct FileRequestHandler {
    base_path:                  String,
    allow_directory_listing:    bool,
}

impl FileRequestHandler {
    pub fn new() -> FileRequestHandler {
        FileRequestHandler { base_path: String::new(), allow_directory_listing: false }
    }
}

impl SubRequestHandler for FileRequestHandler {
    fn configure(&mut self, site_config: &SiteConfig, _config: &Configuration) -> bool {
        self.base_path = site_config.get_param("basePath");
        
        !self.base_path.is_empty()
    }

    fn handle_request(&self, connection: &RequestConnection, _request: &WebRequest, remaining_uri: &str) -> HandleRequestResult {
        let base_path = Path::new(&self.base_path);
        let target_path = base_path.join(remaining_uri).to_str().unwrap().to_string();

        let wrg = WebResponseGeneratorFile::new(target_path);

        let response = wrg.get_response_string();

        let mut stream = &connection.tpc_stream;
    
        stream.write(response.as_bytes()).unwrap();
        stream.flush().unwrap();

        HandleRequestResult::RequestHandledOK
    }
}
