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

use std::collections::BTreeMap;

use std::cmp;

use std::sync::Arc;

use chrono::Datelike;

use super::photo_item::PhotoItem;

#[derive(Clone, Debug, Eq)]
pub struct YearMonth {
    year:   u32,
    month:  u32
}

impl YearMonth {
    pub fn new(year: u32, month: u32) -> YearMonth {
        YearMonth { year, month }
    }
}

impl PartialEq for YearMonth {
    fn eq(&self, other: &Self) -> bool {
        self.year == other.year && self.month == other.month
    }
}

impl PartialOrd for YearMonth {
    fn partial_cmp(&self, other: &Self) -> Option<cmp::Ordering> {
        // TODO: less verbose equiv?
        if self.year == other.year && self.month == other.month {
            Some(cmp::Ordering::Equal)
        }
        else {
            if self.year == other.year {
                Some(self.month.cmp(&other.month))
            }
            else {
                Some(self.year.cmp(&other.year))
            }
        }
    }
}

impl Ord for YearMonth {
    fn cmp(&self, other: &Self) -> cmp::Ordering {
        return self.partial_cmp(other).unwrap();
    }
}

#[derive(Clone, Debug)]
pub struct PhotoResultsDateAccessor {
    pub year_items:             BTreeMap<u32, Vec<Arc<Box<PhotoItem>>>>,
    pub year_month_items:       BTreeMap<YearMonth, Vec<Arc<Box<PhotoItem>>>>,
    pub year_month_indicies:    BTreeMap<u32, Vec<u32>>,
}

impl PhotoResultsDateAccessor {
    pub fn new() -> PhotoResultsDateAccessor {
        PhotoResultsDateAccessor { year_items: BTreeMap::new(), year_month_items: BTreeMap::new(),
                 year_month_indicies: BTreeMap::new() }
    }

    pub fn build(&mut self, photos: &Vec<Arc<Box<PhotoItem>>>) {
        for photo in photos {
            if photo.time_taken.is_none() {
                continue;
            }

            // we've got a date, so work out the year and month of it

            let year = photo.time_taken.unwrap().year() as u32;
            let month = photo.time_taken.unwrap().month();
            // in the Rust version, Chrono uses 1-based months, so we need to negate it by 1 to match
            // C++ version, although we might want to update both so that 1-based versions at the URI
            // level are used?
            let month = month - 1;

            let year_item = &mut self.year_items.get_mut(&year);
            if year_item.is_none() {
                // we don't have a key for this one yet
                let mut new_vec = Vec::new();
                new_vec.push(photo.clone());

                self.year_items.insert(year, new_vec);

                let year_new_vector = Vec::new();
                self.year_month_indicies.insert(year, year_new_vector);
            }
            else {
                let items = year_item.as_mut().unwrap();
                items.push(photo.clone());
            }

            let year_month = YearMonth::new(year, month);

            let year_month_item = &mut self.year_month_items.get_mut(&year_month);
            if year_month_item.is_none() {
                // we don't have a key for this one yet
                let mut new_vec = Vec::new();
                new_vec.push(photo.clone());

                self.year_month_items.insert(year_month, new_vec);

                let mut year_index = self.year_month_indicies.get_mut(&year);
                let months_for_year = year_index.as_mut().unwrap();
                months_for_year.push(month);
            }
            else {
                let items = year_month_item.as_mut().unwrap();
                items.push(photo.clone());
            }
        }
    }

    pub fn get_list_of_years(&self) -> Vec<u32> {
        let list_of_years = self.year_month_indicies.keys().cloned().collect();
        list_of_years
    }

    pub fn get_list_of_months_for_year(&self, year: u32) -> &Vec<u32> {
        let months = self.year_month_indicies.get(&year).unwrap();
        months
    }

    pub fn get_photos_for_year(&self, year: u32) -> &Vec<Arc<Box<PhotoItem>>> {
        let photos = self.year_items.get(&year).unwrap();
        photos
    }

    pub fn get_photos_for_month_year(&self, year: u32, month: u32) -> &Vec<Arc<Box<PhotoItem>>> {
        let photos = self.year_month_items.get(&YearMonth::new(year, month)).unwrap();
        photos
    }
}
