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

use chrono::prelude::*;

use super::photo_representations::PhotoRepresentations;

#[derive(Clone, Copy, Debug)]
#[repr(u32)]
#[allow(dead_code)]
pub enum SourceType {
    Unknown     = 0,
    SLR         = 1 << 0,
    Phone       = 1 << 1,
    Compact     = 1 << 2,
    Drone       = 1 << 3,
}

#[derive(Clone, Copy, Debug)]
#[repr(u32)]
#[allow(dead_code)]
pub enum ItemType {
    Unknown      = 0,
    Still        = 1 << 0,
    Movie        = 1 << 1,
    Panorama     = 1 << 2,
    Spherical360 = 1 << 3,
    Timelapse    = 1 << 4,
}

#[derive(Clone, Copy, Debug, PartialEq, PartialOrd)]
#[repr(u32)]
#[allow(dead_code)]
pub enum PermissionType {
    Public,
    AuthorisedBasic,
    AuthorisedAdvanced,
    Private,
}

#[derive(Clone, Debug)]
pub struct PhotoItem {
pub representations:    PhotoRepresentations,

pub time_taken:         Option<NaiveDateTime>,

pub source_type:        SourceType,
pub item_type:          ItemType,

pub permission_type:    PermissionType,

pub rating:             u8,

// TODO: make this a string token to be more memory-efficient...
pub geo_location_path:  String,
}

impl PhotoItem {
    pub fn new() -> PhotoItem {
        PhotoItem { representations: PhotoRepresentations::new(), time_taken: None,
                    source_type: SourceType::Unknown, item_type: ItemType::Unknown, permission_type: PermissionType::Public,
                    rating: 0, geo_location_path: String::new() }
    }

    // this version is only set from the date string in the .txt file, and will in most cases
    // be later overridden by the full date/time combo from the EXIF info of the photo itself...
    pub fn set_basic_date(&mut self, date: &String) {
        self.time_taken = Some(NaiveDate::parse_from_str(date, "%Y-%m-%d").unwrap().and_hms(1, 1, 1));
    }

    // TODO: do this properly with a dedicated struct at some point...
    pub fn set_info_from_exif(&mut self, datetime: &str) {
        self.time_taken = Some(NaiveDateTime::parse_from_str(datetime, "%Y-%m-%d %H:%M:%S").unwrap());
    }


}
