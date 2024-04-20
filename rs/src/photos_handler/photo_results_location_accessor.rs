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

use std::sync::Arc;

use super::photo_item::PhotoItem;

#[derive(Clone, Debug)]
struct LocationHierarchyItem {
    name:                   String,
    description:            String,

    sub_location_lookup:    BTreeMap<String, u32>,
    photos:                 Vec<Arc<Box<PhotoItem>>>
}

impl LocationHierarchyItem {
    pub fn new(name: &str) -> LocationHierarchyItem {
        LocationHierarchyItem { name: name.to_string(), description: String::new(),
                                sub_location_lookup: BTreeMap::new(), photos: Vec::new() }
    }
}

#[derive(Clone, Debug)]
pub struct PhotoResultsLocationAccessor {
    items:                  Vec<LocationHierarchyItem>,

    location_lookup:        BTreeMap<String, u32>
}

impl PhotoResultsLocationAccessor {
    pub fn new() -> PhotoResultsLocationAccessor {
        PhotoResultsLocationAccessor { items: Vec::new(), location_lookup: BTreeMap::new() }
    }

    pub fn build(&mut self, photos: &Vec<Arc<Box<PhotoItem>>>) {
        for photo in photos {
            // TODO: do we care about this for locations?
            if photo.time_taken.is_none() {
                continue;
            }

            if photo.geo_location_path.is_empty() {
                continue;
            }

            let location_string = photo.geo_location_path.trim();

            let location_components: Vec<&str> = location_string.split('/').map(|x| x.trim()).collect();

            let mut new_temp_items : Vec<usize> = Vec::with_capacity(4);

            let mut level = 0;
            for comp_string in location_components {
                if level == 0 {
                    let find_comp = self.location_lookup.get(comp_string);
                    if let Some(comp) = find_comp {
                        // we've seen this first level item before...
                        let first_level_component_item_index = comp;

                        new_temp_items.push(*first_level_component_item_index as usize);
                    }
                    else {
                        // we didn't find it, so we need to create it...
                        let new_item_index = self.items.len();

                        self.items.push(LocationHierarchyItem::new(comp_string));

                        self.location_lookup.insert(comp_string.to_string(), new_item_index as u32);

                        new_temp_items.push(new_item_index);
                    }
                }
                else {
                    // do the remaining items...

                    let previous_level_item_index = new_temp_items[level - 1_usize];
                    let previous_level_item = &self.items[previous_level_item_index];
                    let find_comp = previous_level_item.sub_location_lookup.get(comp_string);

                    if let Some(comp) = find_comp {
                        // we've seen this level item before...
                        let this_sub_item_index = comp;

                        new_temp_items.push(*this_sub_item_index as usize);
                    }
                    else {
                        // we didn't find it, so we need to create it...
                        let new_item_index = self.items.len();

                        self.items.push(LocationHierarchyItem::new(comp_string));

                        let previous_level_item = &mut self.items[previous_level_item_index];
                        previous_level_item.sub_location_lookup.insert(comp_string.to_string(), new_item_index as u32);

                        new_temp_items.push(new_item_index);
                    }
                }

                level += 1;
            }

            // now we should have an array of the items to add the photo to...

            for ind in new_temp_items {
                let location_item = &mut self.items[ind];
                location_item.photos.push(photo.clone());
            }
        }
    }

    pub fn get_photos_for_location(&self, location_path: &str) -> Option<&Vec<Arc<Box<PhotoItem>>>> {
        if location_path.is_empty() {
            return None;
        }

        let location_components: Vec<&str> = location_path.split('/').map(|x| x.trim()).collect();

        let mut item: Option<&LocationHierarchyItem> = None;
        let mut component_count = 0;

        for component in location_components {
            if component_count == 0 {
                if let Some(index) = self.location_lookup.get(component) {
                    item = Some(&self.items[*index as usize]);
                }
                else {
                    return None;
                }
            }
            else if let Some(sub_item) = item {
                if let Some(index) = sub_item.sub_location_lookup.get(component) {
                    item = Some(&self.items[*index as usize]);
                }
            }
            else {
                return None;
            }

            component_count += 1;
        }

        if let Some(item_val) = item {
            return Some(&item_val.photos);
        }

        None
    }

    pub fn get_sub_locations_for_location(&self, location_path: &str) -> Vec<String> {
        let mut sub_locations = Vec::new();

        if location_path.is_empty() {
            // we're at the root, so return the first level items...
            for item in self.location_lookup.keys() {
                sub_locations.push(item.clone());
            }
            return sub_locations;
        }

        // otherwise, follow the components...
        let location_components: Vec<&str> = location_path.split('/').map(|x| x.trim()).collect();

        let mut item: Option<&LocationHierarchyItem> = None;
        let mut component_count = 0;

        for component in location_components {
            if component_count == 0 {
                if let Some(index) = self.location_lookup.get(component) {
                    item = Some(&self.items[*index as usize]);
                }
                else {
                    return sub_locations;
                }
            }
            else if let Some(sub_item) = item {
                if let Some(index) = sub_item.sub_location_lookup.get(component) {
                    item = Some(&self.items[*index as usize]);
                }
            }
            else {
                return sub_locations;
            }

            component_count += 1;
        }

        if let Some(item_val) = item {
            for item in item_val.sub_location_lookup.keys() {
                sub_locations.push(item.clone());
            }
        }

        sub_locations
    }
}
