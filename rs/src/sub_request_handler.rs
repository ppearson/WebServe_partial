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


use crate::configuration::{Configuration, SiteConfig};
use crate::web_request::WebRequest;
use crate::web_server_common::RequestConnection;

#[derive(Clone, Copy, Debug)]
pub enum HandleRequestResult {
    RequestHandledOK,
    RequestHandledDisconnect,
    RequestHandledError,
    RequestNotHandled
}

impl HandleRequestResult {
    pub fn was_handled(&self) -> bool {
        !matches!(*self, HandleRequestResult::RequestNotHandled)
    }

    pub fn was_error(&self) -> bool {
        matches!(*self, HandleRequestResult::RequestHandledDisconnect) || matches!(*self, HandleRequestResult::RequestHandledError)
    }
}

pub trait SubRequestHandler {
    fn configure(&mut self, site_config: &SiteConfig, config: &Configuration) -> bool;

    fn handle_request(&self, connection: &RequestConnection, request: &WebRequest, remaining_uri: &str) -> HandleRequestResult;
}
