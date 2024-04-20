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

#![allow(dead_code)]

use std::fmt;

use std::io::{BufRead, BufReader};

use std::collections::BTreeMap;

#[derive(Clone, Debug)]
pub struct SiteConfig {
pub name:           String,
pub ctype:          String,

pub definition:     String,

pub params:         BTreeMap<String, String>
}

impl Default for SiteConfig {
    fn default () -> SiteConfig {
        SiteConfig{name: String::new(), ctype: String::new(), definition: String::new(),
                    params: BTreeMap::new()}
    }
}

impl fmt::Display for SiteConfig {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Name: {}, type: {}", self.name, self.ctype)
    }
}

impl SiteConfig {

    pub fn has_param(&self, param: &str) -> bool {
        self.params.contains_key(param)
    }

    pub fn get_param(&self, param: &str) -> String {
        let mut ret_value = String::new();

        if self.params.contains_key(param) {
            ret_value = self.params.get(param).unwrap().clone();
        }

        ret_value
    }

    pub fn get_param_as_bool(&self, param: &str, default: bool) -> bool {
        let result = self.params.get(param);
        match result {
            None => return default,
            Some(val) => {
                if val == "1" || val == "true" || val == "yes" {
                    true
                }
                else {
                    false
                }
            }
        }
    }

    pub fn get_param_as_uint(&self, param: &str, default: u32) -> u32 {
        let result = self.params.get(param);
        match result {
            None => default,
            Some(val) => {
                let uint_value = val.parse::<u32>().unwrap();
                uint_value
            }
        }
    }
}

#[derive(Clone, Debug)]
pub struct Configuration {
pub worker_threads:                 u32,
pub enable_http:                    bool,
pub port_number_http:               u32,

pub downgrade_user_after_bind:      bool,
pub downgrade_username:             String,

pub keep_alive_enabled:             bool,
pub keep_alive_timeout:             u32,
pub keep_alive_limit:               u32,

pub site_configs:                   Vec<SiteConfig>
}

#[cfg(target_os = "macos")]
const DEFAULT_CONFIG_PATH: &str = "/Users/peter/webserve.ini";
#[cfg(target_os = "linux")]
const DEFAULT_CONFIG_PATH: &str = "/home/peter/webserve.ini";

impl Configuration {
    pub fn new() -> Configuration {
        Configuration{ worker_threads: 8,
                        enable_http: true,
                        port_number_http: 9393,
                        downgrade_user_after_bind: false,
                        downgrade_username: "".to_string(),
                        keep_alive_enabled: false,
                        keep_alive_timeout: 6,
                        keep_alive_limit: 20,
                        site_configs: Vec::with_capacity(0) }
    }

    pub fn auto_load_file(&mut self) -> bool {
        self.load_from_file(&DEFAULT_CONFIG_PATH)
    }

    // TODO: return Result instead...
    pub fn load_from_file(&mut self, filename: &str) -> bool {
        let file = std::fs::File::open(filename).unwrap();
        let reader = BufReader::new(file);

        // TODO: error handling...

        for line in reader.lines() {
            let line = line.unwrap();

            // ignore empty lines and comments
            if line.is_empty() || line.starts_with('#') {
                continue;
            }

            // first of all, see if it's a site definition statement
            if line.contains(" =") && line.contains("site(") {
                // it's a site config definition statement, i.e.:
                // <name> = site(<type>, <config>)
                // "photos = site(photos, host:photos.mydomain.net)

                self.site_configs.push(SiteConfig::default());
                let mut current_site_config = self.site_configs.last_mut().unwrap();

                let name_pos = line.find(' ').unwrap();
                let name = line[0..name_pos].to_string();

                current_site_config.name = name.clone();

                // println!("New Site: {}", name);

                if let Some(site_def_start) = line.find("site(") {
                    if let Some(site_def_end) = line.find(')') {
                        if site_def_start < site_def_end {
                            let site_def_start = site_def_start + 5;
                            let site_def_contents = line[site_def_start..site_def_end].to_string();

                            let (def_type, def_config) = site_def_contents.split_once(',').unwrap();
                            let def_type = def_type.trim();
                            let def_config = def_config.trim();
                            current_site_config.ctype = def_type.to_string();
                            current_site_config.definition = def_config.to_string();
                        }
                        else {
                            eprintln!("Warning: malformed site definition in config file...");
                        }
                    }
                    else {
                        eprintln!("Warning: malformed site definition in config file...");
                    }
                }

                // skip this line, we're done...
                continue;
            }

            // see if we're setting a param for a site, with a '.' in the initial name, i.e.
		    // photos.webContentPath: 1
            if line.contains(": ") {
                if let Some(colon_pos) = line.find(':') {
                    if let Some(dot_pos) = line.find('.') {
                        if dot_pos < colon_pos {
                            let full_name = line[0..colon_pos].to_string();

                            // site it belongs to - for the moment, we'll ignore this and use the last, but
                            // will need to fix this up in the future.
                            let _site_name = full_name[0..dot_pos].to_string();

                            let param_name = full_name[dot_pos + 1..].to_string();

                            let string_value = line[colon_pos + 1..].to_string();
                            let string_value = string_value.trim().to_string();

                            let current_site_config = self.site_configs.last_mut().unwrap();
                            current_site_config.params.insert(param_name, string_value);
                        }
                        else {
                            eprintln!("Warning: malformed site param definition in config file...");
                        }
                    }
                }
            }

            let mut key = String::new();
            let mut value = String::new();

            if !Configuration::get_key_value(&line, &mut key, &mut value) {
                // TODO: error
                continue;
            }

            if key == "workerThreads" {
                let int_value = value.parse::<u32>().unwrap();
                self.worker_threads = int_value;
            }
            else if Configuration::try_extract_bool_value("enableHTTP", &key, &value, &mut self.enable_http) {

            }
            else if key == "portNumberHTTP" {
                let int_value = value.parse::<u32>().unwrap();
                self.port_number_http = int_value;
            }
            else if Configuration::try_extract_bool_value("downgradeUserAfterBind", &key, &value, &mut self.downgrade_user_after_bind) {

            }
            else if key == "downgradeUserName" {
                if !value.is_empty() {
                    self.downgrade_username = value;
                }
            }
            else if Configuration::try_extract_bool_value("keepAliveEnabled", &key, &value, &mut self.keep_alive_enabled) {

            }
            else if key == "keepAliveTimeout" {
                let int_value = value.parse::<u32>().unwrap();
                self.keep_alive_timeout = int_value;
            }
            else if key == "keepAliveLimit" {
                let int_value = value.parse::<u32>().unwrap();
                self.keep_alive_limit = int_value;
            }
        }

        true
    }

    fn get_key_value(string_val: &str, key: &mut String, value: &mut String) -> bool {
        if !string_val.contains(':') {
            return false;
        }

        // this includes the separator, so...
//        let (key_str, val_str) = string_val.split_at(string_val.find(':').unwrap());
        let mut split = string_val.split(':');
        let (key_str, val_str) = (split.next().unwrap(), split.next().unwrap());

        // TODO: can we just do something like this?
        // key = *key_str.to_owned().to_string();

        key.clear();
        value.clear();

        key.push_str(key_str.trim());
        value.push_str(val_str.trim());

        true
    }

    fn get_bool_value_from_string(string_val: &str, bool_val: &mut bool) -> bool {
        if string_val.is_empty() {
            return false;
        }

        // check known strings first
        if string_val == "true" || string_val == "yes" {
            *bool_val = true;
            return true;
        }
        else if string_val == "false" || string_val == "no" {
            *bool_val = false;
            return true;
        }

        // otherwise, attempt to see if it's a number value, 0 or 1...
        let int_value = string_val.parse::<u32>().unwrap();
        if int_value == 0 {
            *bool_val = false;
            return true;
        }
        else if int_value == 1 {
            *bool_val = true;
            return true;
        }

        false
    }

    fn try_extract_bool_value(try_key_name: &str, actual_key_name: &str,
                              key_value: &str, config_field: &mut bool) -> bool {
        if try_key_name != actual_key_name {
            return false;
        }

        let mut extracted_bool_value = false;
        if Configuration::get_bool_value_from_string(key_value, &mut extracted_bool_value) {
            *config_field = extracted_bool_value;
        }
        else {
            // Error
        }

        // return that the key name actually matched, even if we didn't actually successfully apply the config value...
        true
    }
}
