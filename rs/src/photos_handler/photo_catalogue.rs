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


use std::path::{Path};

use std::sync::Arc;
//use std::sync::Mutex;
//use std::sync::RwLock;

use glob::glob;

use exif::{In, Reader, Tag};

use crate::file_helpers::*;

use super::item_file::{Item, ItemFile};
use super::photo_representations::{PhotoRep};
use super::photo_query_engine::{PhotoQueryEngine};
use super::photo_item::{*};

pub struct PhotoCatalogue {
    pub items:          Vec<Arc<Box<PhotoItem>>>,

    pub query_engine:   PhotoQueryEngine,
}

impl PhotoCatalogue {
    pub fn new() -> PhotoCatalogue {
        return PhotoCatalogue { items: Vec::with_capacity(0),
                                query_engine: PhotoQueryEngine::new() };
    }

    pub fn build_photo_catalogue(&mut self, photos_base_path: &str) {
        self.build_photo_catalogue_from_item_files(photos_base_path);

        self.items.sort_by(|a, b | a.time_taken.partial_cmp(&b.time_taken).unwrap());

        println!("Loaded {} photos...", self.items.len());
    }

    pub fn build_photo_catalogue_from_item_files(&mut self, photos_base_path: &str) {
        println!("Looking for photos in: {}...", photos_base_path);

        let path = Path::new(&photos_base_path);
        let pattern = path.join("**/*.txt").to_str().unwrap().to_string();

        // TODO: parallelise this...

        let mut item_files = Vec::new();

        for item in glob(&pattern).expect("Failed to read glob pattern") {
            match item {
                Ok(path) => {
                    item_files.push(path.display().to_string());
 //                   println!("{:?}", path.display());
                },
                Err(_e) => {},
            }
        }

        // now that we have our list of item files, process them all to get the final baked list
        // of photos that they contain...
        for item_file_path in &item_files {

            let mut item_file = ItemFile::new();

            if !item_file.load_file(&item_file_path) {
                println!("Couldn't load item file: {}...", item_file_path);
                continue;
            }

            let directory_path_of_item_file = Path::new(&item_file_path).parent().unwrap();

            let final_items = item_file.get_final_baked_items();

            for item in &final_items {
                self.process_item_file_item(&photos_base_path.to_string(),
                 &directory_path_of_item_file.to_str().unwrap().to_string(), &item);
            }
        }

    }

    fn process_item_file_item(&mut self, photos_base_path: &String, item_file_directory_path: &String, item: &Item) {

        if !item.has_value(&"res-0-img".to_string()) {
            // if we haven't got a full res property, ignore it completely
		    return;
        }

        // this is inefficient doing this for each item, but in theory it could be different per-item, so not really
	    // sure what else to do...
        let mut item_photo_base_path;
        if item.has_value("basePath") {
            item_photo_base_path = item.get_value("basePath");
            if item_photo_base_path == "." {
                item_photo_base_path = item_file_directory_path.to_string();
            }

            // make it relative if needed.
            remove_prefix_from_path(&mut item_photo_base_path, photos_base_path);
        }
        else {
            item_photo_base_path = photos_base_path.to_string();
        }

        let mut new_item = PhotoItem::new();

        if item.has_value("date") {
            new_item.set_basic_date(&item.get_value("date"));
        }

        new_item.source_type = match item.get_value("sourceType").as_str() {
            "slr" => SourceType::SLR,
            "drone" => SourceType::Drone,
            _ => SourceType::Unknown
        };

        new_item.item_type = match item.get_value("itemType").as_str() {
            "still" => ItemType::Still,
            _ => ItemType::Unknown
        };

        new_item.permission_type = match item.get_value("permission").as_str() {
            "authBasic" => PermissionType::AuthorisedBasic,
            "authAdvanced" => PermissionType::AuthorisedAdvanced,
            "private" => PermissionType::Private,
            _ => PermissionType::Public
        };

        let geo_location_path = item.get_value("geoLocationPath");
        if !geo_location_path.is_empty() {
            new_item.geo_location_path = geo_location_path;
        }

        // try and find all the resolutions we have.
	    // TODO: this isn't really ideal, although as all properties are currently strings, there's not much else we can do,
	    //       although it's something to think about improving in the future...

        for i in 0..6 {
            let res_temp = format!("res-{}", i);
            if !item.has_value(&res_temp) {
                // if we don't have the larger res, it's likely something is wrong with the entire set of this
                // image, so break out..
                break;
            }

            let res_image_value = item.get_value(&format!("{}-img", res_temp));
            if res_image_value.is_empty() {
                continue;
            }

            let item_res_value = item.get_value(&res_temp);

            let relative_image_path = combine_paths(&item_photo_base_path, &res_image_value);

            let full_image_path = combine_paths(&photos_base_path, &relative_image_path);

//            println!("relative image path: {}\nfull_image_path: {}\n", relative_image_path, full_image_path);

            // if we're the main full res (which has been copied from the original), try and extract EXIF info from it
            if i == 0 {
                let file = std::fs::File::open(&full_image_path).unwrap();
                let exif = Reader::new().read_from_container(&mut std::io::BufReader::new(&file)).unwrap();

                let photo_datetime_taken = exif.get_field(Tag::DateTimeDigitized, In::PRIMARY);
                if photo_datetime_taken.is_some() {
                    let photo_date_time_taken_string = photo_datetime_taken.unwrap().display_value().to_string();
                    new_item.set_info_from_exif(&photo_date_time_taken_string);
                }
            }

            let mut have_image_res = false;
            let mut image_width = 0u16;
            let mut image_height = 0u16;
            if !item_res_value.is_empty() {
                // if we've got a res string, assume that's a valid image res and we don't need to open the image...
                if let Some((width, height)) = item_res_value.split_once(',') {
                    image_width = width.parse::<u16>().unwrap();
                    image_height = height.parse::<u16>().unwrap();
                    have_image_res = true;
                }
            }

            let mut image_exists = false;
            if have_image_res {
                // if we have the image res at this point, we didn't necessarily open the image for it, we could
                // have got it from the item file, so double-check the image exists on disk...
                image_exists = std::path::Path::new(&full_image_path).exists();
            }
            else {
                // TODO...
            }

            if !image_exists && i == 0 {
                return;
            }

            new_item.representations.add_representation(PhotoRep::new(&relative_image_path, image_width, image_height));
        }

        self.items.push(Arc::new(Box::new(new_item)));
    }
}

