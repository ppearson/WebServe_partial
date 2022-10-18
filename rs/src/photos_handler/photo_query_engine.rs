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

#![allow(non_camel_case_types)]

use std::sync::Arc;
use std::sync::RwLock;
use super::photo_item::{*};
use super::photo_results::{*};

#[derive(Clone, Debug, PartialEq)]
#[allow(dead_code)]
pub enum QueryType {
    All,
    Year,
    Location
}

/*
#[derive(Clone, Debug, PartialEq)]
#[allow(dead_code)]
pub enum PermissionType {
    Public,
    AuthBasic,
    AuthAdvanced,
    Private
}
*/

#[derive(Clone, Debug, PartialEq)]
#[allow(dead_code)]
pub enum SortOrderType {
    OldestFirst,
    YoungestFirst
}

#[derive(Clone, Copy, Debug)]
#[repr(u32)]
#[allow(dead_code)]
pub enum AccessorBuildFlags {
    BUILD_DATE_ACCESSOR         = 1 << 0,
    BUILD_LOCATION_ACCESSOR     = 1 << 1,
}

#[derive(Clone, Debug, PartialEq)]
pub struct QueryParams {
pub query_type:             QueryType,
pub sort_order_type:        SortOrderType,

    // these are used as bitsets
pub source_types:           u32,
pub item_types:             u32,

pub permission_type:        PermissionType,

    min_rating:             u32,
}

impl QueryParams {
    pub fn new() -> QueryParams {
        QueryParams { query_type: QueryType::All, sort_order_type: SortOrderType::OldestFirst, source_types: 0, item_types: 0,
                      permission_type: PermissionType::Public, min_rating: 0 }
    }

    pub fn make_source_type(want_slr: bool, want_drone: bool) -> u32 {
        let mut val = 0;

        if want_slr {
            val |= SourceType::SLR as u32;
        }
        if want_drone {
            val |= SourceType::Drone as u32;
        }
        return val;
    }
}

const LRU_CACHE_SIZE: usize = 10;

struct LRUCache {
    cached_queries:     Vec<QueryParams>,
    cached_results:     Vec<Arc<Box<PhotoResults>>>,
    num_cached_results: usize,
    next_cache_index:   usize,
}

impl LRUCache {
    pub fn new() -> LRUCache {
        LRUCache { cached_queries: Vec::with_capacity(LRU_CACHE_SIZE), cached_results: Vec::with_capacity(LRU_CACHE_SIZE),
                    num_cached_results: 0, next_cache_index: 0 }
    }
}

pub struct PhotoQueryEngine {
//    items:          Arc<RwLock<Vec<PhotoItem>>>

    cached_items:         RwLock<LRUCache>,
}

impl PhotoQueryEngine {
    pub fn new() -> PhotoQueryEngine {
        PhotoQueryEngine { cached_items: RwLock::new(LRUCache::new()) }
    }

    pub fn get_photo_results(&self, photos: &Vec<Arc<Box<PhotoItem>>>, query_params: QueryParams, build_flags: u32) -> Arc<Box<PhotoResults>> {
        if let Some(cached_res) = self.find_cached_result(&query_params) {
            if build_flags != 0 {
                if build_flags & AccessorBuildFlags::BUILD_DATE_ACCESSOR as u32 != 0 {
                    cached_res.check_date_accessor_is_built();
                }
                else if build_flags & AccessorBuildFlags::BUILD_LOCATION_ACCESSOR as u32 != 0 {
                    cached_res.check_location_accessor_is_built();
                }
            }
            return cached_res;
        }

        let new_result = self.perform_query(photos, &query_params);

        if build_flags != 0 {
            if build_flags & AccessorBuildFlags::BUILD_DATE_ACCESSOR as u32 != 0 {
                new_result.check_date_accessor_is_built();
            }
            else if build_flags & AccessorBuildFlags::BUILD_LOCATION_ACCESSOR as u32 != 0 {
                new_result.check_location_accessor_is_built();
            }
        }

        self.add_cached_result(&query_params, &new_result);

        return new_result;
    }

    fn perform_query(&self, photos: &Vec<Arc<Box<PhotoItem>>>, query_params: &QueryParams) -> Arc<Box<PhotoResults>> {
        let mut results = Box::new(PhotoResults::new());

        for photo in photos {
            if query_params.source_types > 0 {
                if query_params.source_types & photo.source_type as u32 == 0 {
                    continue;
                }
            }

            if query_params.item_types > 0 {
                if query_params.item_types & photo.item_type as u32 == 0 {
                    continue;
                }
            }

            if !matches_permissions(query_params.permission_type, &photo) {
                continue;
            }

            results.results.push(photo.clone());
        }

        if query_params.sort_order_type == SortOrderType::YoungestFirst {
            results.results.reverse();
        }

        let new_result = Arc::new(results);
        return new_result;
    }

    fn find_cached_result(&self, query_params: &QueryParams) -> Option<Arc<Box<PhotoResults>>> {
        let guard = self.cached_items.read().unwrap();
        for i in 0..guard.num_cached_results {
            if query_params == &guard.cached_queries[i] {
                return Some(guard.cached_results[i].clone());
            }
        }

        return None;
    }

    fn add_cached_result(&self, query_params: &QueryParams, results: &Arc<Box<PhotoResults>>) {
        let mut mut_guard = self.cached_items.write().unwrap();
        if mut_guard.num_cached_results < LRU_CACHE_SIZE {
            // we haven't hit the limit yet, so just add it to the end...
            let next_index = mut_guard.next_cache_index;
            assert!(next_index == mut_guard.cached_queries.len());
            mut_guard.cached_queries.push(query_params.clone());
            mut_guard.cached_results.push(results.clone());

            mut_guard.next_cache_index += 1;
            mut_guard.num_cached_results += 1;
        }
        else {
            // we're going to have to replace one of the existing ones..
            let next_val = (mut_guard.next_cache_index + 1) % LRU_CACHE_SIZE;
            mut_guard.cached_queries[next_val] = query_params.clone();
            mut_guard.cached_results[next_val] = results.clone();

            mut_guard.next_cache_index = next_val;
        }
    }
}

fn matches_permissions(permission: PermissionType, item: &PhotoItem) -> bool {
    if item.permission_type == PermissionType::Public {
        return true;
    }

    if item.permission_type > permission {
        return false;
    }

    return true;
}
