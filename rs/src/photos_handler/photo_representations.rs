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


#[derive(Clone, Debug)]
pub struct PhotoRep {

pub relative_file_path:     String,
pub width:                  u16,
pub height:                 u16,
pub aspect_ratio:           f32,
}

impl PhotoRep {
    pub fn new(relative_path: &str, width: u16, height: u16) -> PhotoRep {
        let aspect_ratio = (width as f32) / (height as f32);
        PhotoRep { relative_file_path: relative_path.to_string(), width, height, aspect_ratio }
    }
}

#[derive(Clone, Debug)]
pub struct PhotoRepresentations {

    pub representations:    Vec<PhotoRep>
}

impl PhotoRepresentations {
    pub fn new() -> PhotoRepresentations {
        PhotoRepresentations { representations: Vec::with_capacity(0 )}
    }

    pub fn add_representation(&mut self, photo_rep: PhotoRep) {
        self.representations.push(photo_rep);
    }

    pub fn get_smallest_representation_matching_criteria_min_dimension(&self, min_val: u32) -> Option<&PhotoRep> {

        let mut smallest_val_found = 12000_u16;
        let mut smallest_index = -1i32;

        let mut index = 0;
        for repr in &self.representations {
            let max_dim = std::cmp::max(repr.width, repr.height);

            if max_dim >= min_val as u16 && (max_dim as u16) < smallest_val_found {
                smallest_val_found = max_dim;
                smallest_index = index;
            }
            index += 1;
        }

        if smallest_index == -1 {
            return None;
        }
        else {
            return Some(&self.representations[smallest_index as usize]);
        }
    }

    pub fn get_first_representation_matching_criteria_min_dimension(&self, min_val: u32, return_largest_if_not_found: bool) -> Option<&PhotoRep> {
        let mut largest_val_found = 1_i32;
        let mut largest_index = -1_i32;

        let mut index = 0;
        for repr in &self.representations {
            let min_dim = std::cmp::min(repr.width, repr.height);
            if min_dim >= min_val as u16 {
                return Some(repr);
            }

            if min_dim > largest_val_found as u16 {
                largest_val_found = min_dim as i32;
                largest_index = index;
            }
            index += 1;
        }

        if !return_largest_if_not_found {
            // we didn't find anything...
            return None;
        }

        // otherwise, return the largest we previously found...
        if largest_index != -1 {
            return Some(&self.representations[largest_index as usize]);
        }

        return None;
    }
}
