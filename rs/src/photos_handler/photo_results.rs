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

use std::sync::Arc;
use std::sync::RwLock;
use std::sync::atomic::{AtomicBool, Ordering};

use super::photo_item::{PhotoItem};
use super::photo_results_date_accessor::{PhotoResultsDateAccessor};
use super::photo_results_location_accessor::{PhotoResultsLocationAccessor};

#[derive(Debug)]
pub struct PhotoResults {
pub have_results:               bool,
pub results:                    Vec<Arc<Box<PhotoItem>>>,

pub date_accessor_built:        AtomicBool,
pub date_accessor:              RwLock<PhotoResultsDateAccessor>,

pub location_accessor_built:    AtomicBool,
pub location_accessor:          RwLock<PhotoResultsLocationAccessor>
}

impl PhotoResults {
    pub fn new() -> PhotoResults {
        PhotoResults { have_results: false, results: Vec::with_capacity(0),
                        date_accessor_built: AtomicBool::new(false),
                        date_accessor: RwLock::new(PhotoResultsDateAccessor::new()),
                        location_accessor_built: AtomicBool::new(false),
                        location_accessor: RwLock::new(PhotoResultsLocationAccessor::new()) }
    }

    pub fn check_date_accessor_is_built(&self) {
        if self.date_accessor_built.load(Ordering::Relaxed) {
            return;
        }

        self.date_accessor.write().unwrap().build(&self.results);
        self.date_accessor_built.store(true, Ordering::Relaxed);
    }

    pub fn check_location_accessor_is_built(&self) {
        if self.location_accessor_built.load(Ordering::Relaxed) {
            return;
        }

        self.location_accessor.write().unwrap().build(&self.results);
        self.location_accessor_built.store(true, Ordering::Relaxed);
    }
}
