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

#![allow(dead_code)]
#![allow(non_snake_case)]

use std::collections::BTreeMap;

use crate::string_helpers;

#[derive(Clone, Copy, Debug)]
#[derive(PartialEq)]
pub enum HTTPVersion {
    HTTPUnknown,
    HTTP09,
    HTTP10,
    HTTP11
}

#[derive(Clone, Copy, Debug)]
#[derive(PartialEq)]
pub enum HTTPRequestType {
    Unknown,
    GET,
    POST,
    HEAD
}

#[derive(Clone, Copy, Debug)]
#[derive(PartialEq)]
pub enum ConnectionType {
    ConnectionUnknown,
    ConnectionClose,
    ConnectionKeepAlive
}

#[derive(Clone, Debug)]
#[derive(PartialEq)]
pub struct WebRequest {
    raw_request:        String,

    // extracted field strings
    user_agent_field:   String,

    // decoded / processed items...
 pub request_type:       HTTPRequestType,
 pub request_version:    HTTPVersion,

 pub path:               String,

 pub host_value:         String,
    connection_value:   String,

 pub connection_type:    ConnectionType,

    params:             BTreeMap<String, String>,
    cookies:            BTreeMap<String, String>,
}

#[derive(Clone, Copy, Debug)]
#[derive(PartialEq)]
pub enum WebRequestParseError {
    InvalidRequest
}

pub type ParseResult = Result<WebRequest, WebRequestParseError>;

impl Default for WebRequest {
    fn default () -> WebRequest {
        WebRequest{raw_request: String::new(), user_agent_field: String::new(),
                    request_type: HTTPRequestType::Unknown, request_version: HTTPVersion::HTTPUnknown,
                    path: String::new(),
                    host_value: String::new(),
                    connection_value: String::new(),
                    connection_type: ConnectionType::ConnectionUnknown,
                    params: BTreeMap::new(), cookies: BTreeMap::new()
                }
    }
}

fn extract_field_item(field_string: &str, start_pos: usize) -> &str {
    let value = &field_string[start_pos..];
    return value;
}

impl WebRequest {

    pub fn create_from_string(request: &str) -> ParseResult {
        let raw_request = request.clone();

        // TODO: doing it this way is very hacky as it means we lose blank lines (end of header, etc)...

        let mut request_lines = raw_request.lines();

        // first line should contain main request of "<COMMAND> <PATH> <HTTP_VER>"...
        let first_line = request_lines.next().unwrap();
        let request_components: Vec<&str> = first_line.splitn(3, ' ').collect();
        if request_components.len() != 3 {
            return Err(WebRequestParseError::InvalidRequest);
        }

        let http_command_string = request_components[0];
        let request_type = match http_command_string {
            "GET" => HTTPRequestType::GET,
            "POST" => HTTPRequestType::POST,
            "HEAD" => HTTPRequestType::HEAD,
            _   => HTTPRequestType::Unknown
        };

        let http_version_string = &request_components[2];
        if http_version_string.len() != 8 {
            return Err(WebRequestParseError::InvalidRequest);
        }
        if !http_version_string.starts_with("HTTP/") {
            return Err(WebRequestParseError::InvalidRequest);
        }
        // advance to actual version part, from 5th char for 3 chars...
        let http_version_string_token = &http_version_string[5..][..3];

        let request_version = match http_version_string_token {
            "0.9" => HTTPVersion::HTTP09,
            "1.0" => HTTPVersion::HTTP10,
            "1.1" => HTTPVersion::HTTP11,
            _     => HTTPVersion::HTTPUnknown
        };

        let mut connection_type = ConnectionType::ConnectionUnknown;

        if request_version == HTTPVersion::HTTP11 {
            connection_type = ConnectionType::ConnectionKeepAlive;
        }

        let mut path = request_components[1].to_string();
        // TODO: look for GET params after '?'

        let mut params = BTreeMap::new();

        let question_mark_pos = path.find('?');
        if question_mark_pos.is_some() {
            if request_type == HTTPRequestType::GET {
                let get_params_string = &path[question_mark_pos.unwrap()+1..].to_string();
                params = WebRequest::process_get_params(&get_params_string);
            }
            path = path[..question_mark_pos.unwrap()].to_string();
        }

        let mut found_host = false;
        let mut found_connection = false;
        let mut found_cookie = false;

        let mut host_value = String::new();
        let mut cookies = BTreeMap::new();

        // now process all other lines...
        // TODO: something more efficient than this...
        // TODO: and might need to be case-insensitive?
        for line in request_lines {
            if !found_host && line.starts_with("Host:") {
                host_value = extract_field_item(&line, 5 + 1).to_string();
                found_host = true;
            }
            else if !found_cookie && line.starts_with("Cookie:") {
                let cookie_value = extract_field_item(&line, 7 + 1).to_string();
                cookies = WebRequest::process_cookie_string(&cookie_value);
                found_cookie = true;
            }
            else if !found_connection && line.starts_with("Connection:") {
                let connection_value = extract_field_item(&line, 11 + 1).to_string();
                match connection_value.as_str() {
                    "close" => {
                        connection_type = ConnectionType::ConnectionClose;
                    }
                    "keep-alive" => {
                        connection_type = ConnectionType::ConnectionKeepAlive;
                    }
                    _ => {}
                }
                found_connection = true;
            }
        }

        Ok(WebRequest { raw_request: raw_request.to_string(), request_type, request_version, path, host_value, connection_type, params, cookies, ..Default::default() })
    }

    pub fn has_param(&self, param: &str) -> bool {
        return self.params.contains_key(param);
    }

    pub fn get_param(&self, param: &str) -> String {
        let mut ret_value = String::new();

        if self.params.contains_key(param) {
            ret_value = self.params.get(param).unwrap().clone();
        }

        return ret_value;
    }

    pub fn get_param_as_uint(&self, param: &str, default: u32) -> u32 {
        let result = self.params.get(param);
        match result {
            None => return default,
            Some(val) => {
                let uint_value = val.parse::<u32>().unwrap();
                return uint_value;
            }
        }
    }

    pub fn get_params_as_GET_string(&self, ignore_pagination_params: bool) -> String {
        let mut params_string = String::new();

        // TODO: can we use iterators + fold/map for this?
        for (k,v) in &self.params {
            if ignore_pagination_params && (k == "perPage" || k == "startIndex") {
                continue;
            }

            if !params_string.is_empty() {
                params_string.push('&');
            }

            // TODO: hex/string/encoding of things...
            params_string.push_str(&format!("{}={}", k, v));
        }

        return params_string;
    }

    pub fn has_cookie(&self, cookie: &str) -> bool {
        return self.cookies.contains_key(cookie);
    }

    pub fn get_cookie(&self, cookie: &str) -> String {
        let mut ret_value = String::new();

        if self.cookies.contains_key(cookie) {
            ret_value = self.cookies.get(cookie).unwrap().clone();
        }

        return ret_value;
    }

    pub fn get_cookie_as_uint(&self, cookie: &str, default: u32) -> u32 {
        let result = self.cookies.get(cookie);
        match result {
            None => return default,
            Some(val) => {
                let uint_value = val.parse::<u32>().unwrap();
                return uint_value;
            }
        }
    }

    pub fn get_param_or_cookie_as_uint(&self, param_name: &str, cookie_name: &str, default: u32) -> u32 {
        let result = self.params.get(param_name);
        match result {
            Some(val) => {
                let uint_value = val.parse::<u32>().unwrap();
                return uint_value;
            }
            None => {
                return self.get_cookie_as_uint(cookie_name, default);
            }   
        }
    }

    fn process_get_params(params_string: &str) -> BTreeMap<String, String> {
        let mut params = BTreeMap::new();

        for item in params_string.split('&') {
            if let Some((key, val)) = item.split_once('=') {
                
                let val = &string_helpers::simple_decode_string(&val);

                if !key.is_empty() && !val.is_empty() {
                    params.insert(key.to_string(), val.to_string());
                }
            }
        }

        return params;
    }

    fn process_cookie_string(cookie_string: &str) -> BTreeMap<String, String> {
        let mut cookies = BTreeMap::new();

        // TODO: this probably isn't completely robust to all types of strings, even if they are hex-encoded...
        for item in cookie_string.split("; ") {
            if let Some((key, val)) = item.split_once('=') {
                if !key.is_empty() && !val.is_empty() {
                    cookies.insert(key.to_string(), val.to_string());
                }
            }
        }

        return cookies;
    }
}


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn request_main_invalid() {
        let req_string = "/ g HTTP1.1\r\n".to_string();

        let web_request = WebRequest::create_from_string(&req_string);

        assert!(web_request.is_err(), "web_request should be an error...");
        assert_eq!(web_request, Err(WebRequestParseError::InvalidRequest));
    }

    #[test]
    fn request_main_version1() {
        let req_string = "GET / HTTP/1.1\r\nHost: test.com\r\n".to_string();

        let web_request = WebRequest::create_from_string(&req_string).unwrap();

        assert_eq!(web_request.request_type, HTTPRequestType::GET);
        assert_eq!(web_request.path, "/");
        assert_eq!(web_request.request_version, HTTPVersion::HTTP11);

        assert_eq!(web_request.host_value, "test.com");
    }

    #[test]
    fn request_main_version2() {
        let req_string = "POST /submit?force=true HTTP/1.1\r\nHost: test.com\r\n";

        let web_request = WebRequest::create_from_string(&req_string).unwrap();

        assert_eq!(web_request.request_type, HTTPRequestType::POST);
        assert_eq!(web_request.path, "/submit");
        assert_eq!(web_request.request_version, HTTPVersion::HTTP11);

        assert_eq!(web_request.host_value, "test.com");
    }

    

}
