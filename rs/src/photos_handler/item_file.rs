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

use std::io::{BufRead, BufReader};

use std::collections::BTreeMap;

use crate::string_helpers;

static COMMON_OVERRIDE_FLAG_MASK : u32  = 1 << 31;
const SET_KEY_NAMES : [&str; 2] = ["tags", "geoLocationTags"];

#[derive(Clone, Debug)]
pub struct Item {
    pub values:         BTreeMap<String, String>
}

impl Item {
    pub fn new() -> Item {
        return Item { values: BTreeMap::new() };
    }

    pub fn has_value(&self, key: &str) -> bool {
        return self.values.contains_key(key);
    }

    pub fn get_value(&self, key: &str) -> String {
        let res = match self.values.get(key) {
            Some(s) => s.to_string(),
            None => String::new()
        };

        res
    }
}

#[derive(Clone, Debug)]
pub struct ItemFile {

    // key/values applied to all items - although items can themselves then override these on a per-item basis
    common_values:              BTreeMap<String, String>,
    overrides:                  Vec<BTreeMap<String, String>>,
    value_items:                Vec<Item>,

    // actual indices to items, with left-most bit indicating whether it's an item or override at the position.
    item_indices:               Vec<u32>,
}

impl ItemFile {
    pub fn new() -> ItemFile {
        ItemFile { common_values: BTreeMap::new(), overrides: Vec::with_capacity(0),
                            value_items: Vec::with_capacity(0), item_indices: Vec::with_capacity(0) }
    }

    pub fn load_file(&mut self, file_path: &str) -> bool {

        let file = std::fs::File::open(file_path).unwrap();
        let reader = BufReader::new(file);

        // TODO: error handling...

        let mut temp_item = Item::new();
        let mut item_count = 0;
        let mut have_new_item = false;

        for line in reader.lines() {
            let line = line.unwrap();

            // ignore empty lines and comments
            if line.len() == 0 || line.starts_with('#') {
                continue;
            }

            if line.starts_with('*') {
                // it's a new item

                item_count += 1;

                // if we've previously created a new item, add the previous temp one to the list, in its final state
                if have_new_item {
                    let item_index = self.value_items.len();
                    self.value_items.push(temp_item.clone());
                    self.item_indices.push(item_index as u32);

                    have_new_item = false;
                }

                // clear the per-item key/values.
                temp_item = Item::new();
            }
            else if line.starts_with('\t') {
                let line = &line[1..]; // lose the tab - TODO: make this more robust...

                let res = line.split_once(':');
                if let Some((key, val)) = res {
                    let key = key.trim();
                    if !key.is_empty() {
                        // TODO: might want to be a bit conservative about the below in the future
                        let val = val.trim();

                        temp_item.values.insert(key.to_string(), val.to_string());
                        have_new_item = true;
                    }
                }
            }
            else {
                // it's a common key/value

                let res = line.split_once(':');
                if let Some((key, val)) = res {
                    let key = key.trim();
                    if !key.is_empty() {
                        // TODO: might want to be a bit conservative about the below in the future
                        let val = val.trim();
                        
                        if item_count == 0 {
                            // if we haven't added any items yet, set it as a common value...
                            self.common_values.insert(key.to_string(), val.to_string());
                        }
                        else {
                            // otherwise, it's an override...

                            // if we've previously created a new item, add the previous temp one to the list, in its final state
                            if have_new_item {
                                let item_index = self.value_items.len();
                                self.value_items.push(temp_item.clone());
                                self.item_indices.push(item_index as u32);

                                have_new_item = false;
                            }

                            // add the common override
                            let mut temp_map = BTreeMap::new();
                            temp_map.insert(key.to_string(), val.to_string());

                            // TODO: batching up of any consecutive overrides into one single map item...
                            let override_index = self.overrides.len() as u32;
                            // because it's an override, set the left-most bit
                            let override_index = override_index | COMMON_OVERRIDE_FLAG_MASK;

                            self.overrides.push(temp_map);

                            self.item_indices.push(override_index);
                        }
                    }
                }
            }
        }

        // add any remaining item...
        if have_new_item {
            let item_index = self.value_items.len();
            self.value_items.push(temp_item);
            self.item_indices.push(item_index as u32);
        }

        return true;
    }

    pub fn get_final_baked_items(&self) -> Vec<Item> {

        let mut final_items = Vec::with_capacity(self.item_indices.len());

        // for each item, create a temp copy, assign the common values, and then
        // apply on top any overrides, so we have the final baked values within each item

        // take a copy of the common values, so we can apply common overrides
        let mut local_common_values = self.common_values.clone();
        
        for item_index in &self.item_indices {
            // if it has left-most bit set, it's a common override, otherwise it's just a normal item
            if item_index & COMMON_OVERRIDE_FLAG_MASK == COMMON_OVERRIDE_FLAG_MASK{
                // extract the actual index to the override item
                let override_index = item_index & !COMMON_OVERRIDE_FLAG_MASK;

                let override_item = &self.overrides[override_index as usize];
                for key_val in override_item {
                    local_common_values.insert(key_val.0.to_string(), key_val.1.to_string());
                }
            }
            else {
                // we can just use the index as-is
                let item = &self.value_items[*item_index as usize];

                // create the new item
                let mut temp_item = Item::new();
                temp_item.values = local_common_values.clone();

                for key_val in &item.values {
                    // not great, but...
                    if SET_KEY_NAMES.contains(&key_val.0.as_str()) {
                        // if it's a known set type, don't overwrite it, append it...
                        let current_value = temp_item.values.get_mut(key_val.0).unwrap();
                        *current_value = string_helpers::combine_set_tokens(current_value, key_val.1);
                    }
                    else {
                        temp_item.values.insert(key_val.0.to_string(), key_val.1.to_string());
                    }
                }

                final_items.push(temp_item);
            }
        }

        return final_items;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_Item1() {
        
        let mut item = Item::new();
    //    item.values.insert("basePath", "/home/test1/");
       
      //  assert_eq!(resp_string, expected_response_string);
    }


}
