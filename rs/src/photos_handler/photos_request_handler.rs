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

use crate::configuration::{Configuration, SiteConfig};
use crate::sub_request_handler::{SubRequestHandler, HandleRequestResult};
use crate::web_request::WebRequest;
use crate::web_response_advanced::{WebResponseAdvancedBinaryFile, SendAdvancedResponse};
use crate::web_response_generator::{GetResponseString, WebResponseGeneratorFile,
                                    WebResponseGeneratorTemplateFile};
use crate::web_server_common::RequestConnection;
use crate::photos_handler::photos_common::{DateParams};
use crate::photos_handler::photo_catalogue::PhotoCatalogue;
use crate::photos_handler::photos_html_helpers::{GenMainSitenavCodeParams};
use crate::photos_handler::photos_html_helpers;
use crate::photos_handler::photo_results::{PhotoResults};
use crate::photos_handler::photo_query_engine::{*};
use crate::file_helpers::{combine_paths};
use crate::uri_helpers::{split_first_level_directory_and_remainder};

use std::io::prelude::*;
use std::path::{Path};

pub struct PhotosRequestHandler {
    photos_base_path:                   String,
    main_web_content_path:              String,
    lazy_photo_loading_enabled:         bool,

    // used for <base href> tag in all HTML to make all other paths relative and therefore dynamic depending on whether a domain URL
    // address or directory URL address
    html_base_href:                     String,  //<base href> tag - needs to be "" for domain (no path)

    // relative path of Photos site - from HTTP's perspective (i.e. redirects, direct addressing)
    relative_path:                      String,

    photo_catalogue:                    PhotoCatalogue,
}

impl PhotosRequestHandler {
    pub fn new() -> PhotosRequestHandler {
        PhotosRequestHandler { photos_base_path: String::new(), main_web_content_path: String::new(),
                                lazy_photo_loading_enabled: true,
                                html_base_href: String::new(), relative_path: String::new(),
                                photo_catalogue: PhotoCatalogue::new() }
    }
}

impl SubRequestHandler for PhotosRequestHandler {
    fn configure(&mut self, site_config: &SiteConfig, _config: &Configuration) -> bool {
        self.photos_base_path = site_config.get_param("photosBasePath");
        self.main_web_content_path = site_config.get_param("webContentPath");
        self.lazy_photo_loading_enabled = site_config.get_param_as_bool("lazyPhotoLoadingEnabled", true);

        if !self.photos_base_path.is_empty() {
            self.photo_catalogue.build_photo_catalogue(&self.photos_base_path);
        }

        // TODO: there's duplication here with MainRequestHandler - ought to try and re-use that functionality or
        //       pass down the final results...
        let (site_def_type, site_def_value) = site_config.definition.split_once(':').unwrap();
        if site_def_type == "dir" {
            // set things up so our URI is a directory
			self.html_base_href = format!("<base href=\"/{}/\"/>", site_def_value);
			self.relative_path = format!("/{}/", site_def_value);
        }
        else if site_def_type == "host" {
            // set things up so our URI is a hostname with no directory
			self.html_base_href = "<base href=\"/\"/>".to_string();
			self.relative_path = "/".to_string();
        }
        
        return !self.photos_base_path.is_empty() && !self.main_web_content_path.is_empty();
    }

    fn handle_request(&self, connection: &RequestConnection, request: &WebRequest, remaining_uri: &str) -> HandleRequestResult {
        
        // we received a relative path to photos/<...>

        let request_path = remaining_uri;

        // see if it's a file request - if so, short-circuit it to handle it immediately
	    // TODO: make this more robust
        if request_path.contains('.') {
            // TODO: can this be optimised?
            let extension = Path::new(&request_path).extension().unwrap();
            let extension_lower = extension.to_ascii_lowercase();
            let file_extension = extension_lower.to_str().unwrap();

            let full_path;

            if file_extension == "jpg" {
                full_path = combine_paths(&self.photos_base_path, request_path);

                let wrg = WebResponseAdvancedBinaryFile::new(&full_path);
                if !wrg.send_response(&connection) {
                    // TODO: bit more verification this is the case here?
                    return HandleRequestResult::RequestHandledDisconnect;
                }

                return HandleRequestResult::RequestHandledOK;
            }
            else {
                full_path = combine_paths(&self.main_web_content_path, request_path);
            }

            let wrg = WebResponseGeneratorFile::new(full_path.to_string());

            let response = wrg.get_response_string();

            let mut stream = &connection.tpc_stream;
        
            let write_res = stream.write(response.as_bytes());
            if write_res.is_err() {
                return HandleRequestResult::RequestHandledDisconnect;
            }
            stream.flush().unwrap();

            return HandleRequestResult::RequestHandledOK;
        }

        // otherwise, work out if we have a further first-level sub-directory...
        let mut directory = String::new();
        let mut remainder_uri = String::new();

        let next_level;
        if split_first_level_directory_and_remainder(&request_path, &mut directory, &mut remainder_uri) {
            next_level = directory;
        }
        else {
            next_level = request_path.to_string();
        }

        if next_level == "login" {

        }

        // TODO: authentication...
        if next_level == "photostream" {
            return self.handle_photostream_request(&connection, request);
        }
        else if next_level == "dates" {
            return self.handle_dates_request(&connection, request, &remainder_uri);
        }
        else if next_level == "locations" {
            return self.handle_locations_request(&connection, request);
        }

        // otherwise, handle just general root of photos...
        let site_nav_header_html = photos_html_helpers::generate_main_sitenav_code(GenMainSitenavCodeParams::new(true, true, "",
                                                                                    &self.main_web_content_path));
        let wrg = WebResponseGeneratorTemplateFile::from(&combine_paths(&self.main_web_content_path, "photos_main.tmpl"),
                                &self.html_base_href)
                                .add_field(&site_nav_header_html);

        let response = wrg.get_response_string();

        let mut stream = &connection.tpc_stream;
    
        let write_res = stream.write(response.as_bytes());
        if write_res.is_err() {
            return HandleRequestResult::RequestHandledDisconnect;
        }
        stream.flush().unwrap();

        return HandleRequestResult::RequestHandledOK;
    }
}

impl PhotosRequestHandler {
    fn handle_photostream_request(&self, connection: &RequestConnection, request: &WebRequest) -> HandleRequestResult {
        let mut query_params = QueryParams::new();

        let sort_order_type = match request.get_param_or_cookie_as_uint("sortOrder", "photostream_sortOrderIndex", 1) {
            0 => SortOrderType::OldestFirst,
            _ => SortOrderType::YoungestFirst
        };
        query_params.sort_order_type = sort_order_type;

        let want_slr = request.get_param_or_cookie_as_uint("typeSLR", "photostream_typeSLR", 1) == 1;
        let want_drone = request.get_param_or_cookie_as_uint("typeDrone", "photostream_typeDrone", 0) == 1;

        query_params.source_types = QueryParams::make_source_type(want_slr, want_drone);

        let photo_results = self.photo_catalogue.query_engine.get_photo_results(&self.photo_catalogue.items, query_params, 0);

        let per_page = request.get_param_as_uint("perPage", 100) as usize;
        let start_index = request.get_param_as_uint("startIndex", 0) as usize;
        let slideshow = request.get_param_as_uint("slideshow", 0);

        let site_nav_header_html = photos_html_helpers::generate_main_sitenav_code(GenMainSitenavCodeParams::new(
            slideshow == 0, slideshow == 0, "photostream_", &self.main_web_content_path));
        
        let response;
        if slideshow == 1 {
            let content_and_pagination_html = "<a href=\"javascript:openPhotoSwipe();\">slide show overlay</a><br><br>\n".to_string();
            // TODO:

            let photos_list_js = photos_html_helpers::get_photoswipe_js_item_list(&photo_results.results, start_index, per_page);

            let wrg = WebResponseGeneratorTemplateFile::from(&combine_paths(&self.main_web_content_path, "photostream_slideshow.tmpl"),
                                    &self.html_base_href)
                                    .add_field(&site_nav_header_html)
                                    .add_field(&content_and_pagination_html)
                                    .add_field(&photos_list_js);

            response = wrg.get_response_string();
        }
        else {
            // normal gallery
            let min_thumbnail_size = request.get_param_or_cookie_as_uint("thumbnailSize", "photostream_thumbnailSizeValue", 500);

            let mut pagination_code = String::new();

            if per_page > 0 {
                let total_photos = photo_results.results.len();
                pagination_code = photos_html_helpers::get_pagination_code("photostream/", request, total_photos, start_index,
                                                                             per_page, true, false);
            }

            let lazy_load = self.lazy_photo_loading_enabled && request.get_param_or_cookie_as_uint("lazyLoading", 
                                                                        "photostream_lazyLoading", 1) == 1;      

            let mut slideshow_url = "".to_string();
            if request.get_cookie_as_uint("photostream_galleryLinkToSlideshow", 1) == 1 {
                let current_page_params = request.get_params_as_GET_string(false);
                slideshow_url = format!("photostream/?{}&slideshow=1&", current_page_params);
            }

            let photos_list_html = photos_html_helpers::get_simple_image_list_within_custom_div_tag_with_style(&photo_results.results,
                    "gallery_item", start_index, per_page, min_thumbnail_size, lazy_load, &slideshow_url);

            let wrg = WebResponseGeneratorTemplateFile::from(&combine_paths(&self.main_web_content_path, "photostream_gallery.tmpl"),
                                    &self.html_base_href)
                                    .add_field(&site_nav_header_html)
                                    .add_field(&photos_list_html)
                                    .add_field(&pagination_code);

            response = wrg.get_response_string();
        }

        let mut stream = &connection.tpc_stream;
    
        let write_res = stream.write(response.as_bytes());
        if write_res.is_err() {
            return HandleRequestResult::RequestHandledDisconnect;
        }
        stream.flush().unwrap();
        stream.flush().unwrap();

        return HandleRequestResult::RequestHandledOK;
    }

    fn handle_dates_request(&self, connection: &RequestConnection, request: &WebRequest, remainder_uri: &str) -> HandleRequestResult {
        let mut query_params = QueryParams::new();

        let want_slr = request.get_param_or_cookie_as_uint("typeSLR", "dates_typeSLR", 1) == 1;
        let want_drone = request.get_param_or_cookie_as_uint("typeDrone", "dates_typeDrone", 0) == 1;

        let slideshow = request.get_param_as_uint("slideshow", 0);

        query_params.source_types = QueryParams::make_source_type(want_slr, want_drone);

        let photo_results = self.photo_catalogue.query_engine.get_photo_results(&self.photo_catalogue.items, query_params,
                                     AccessorBuildFlags::BUILD_DATE_ACCESSOR as u32);

        let specify_date_vals_as_dirs = true;

        let date_params = get_date_params_from_request(request, true, remainder_uri);

        let site_nav_header_html = photos_html_helpers::generate_main_sitenav_code(GenMainSitenavCodeParams::new(
            slideshow == 0, slideshow == 0, "dates_", &self.main_web_content_path));

        let response;

        if slideshow == 1 {
            let start_index = request.get_param_as_uint("startIndex", 0) as usize;
            let per_page = request.get_param_as_uint("perPage", 2000) as usize;

            let content_html = "<a href=\"javascript:openPhotoSwipe();\">slide show overlay</a><br><br>\n";

            let photos_list_js;
            if let DateParams::YearAndMonth{ year, month} = date_params {
                let guard = photo_results.date_accessor.read().unwrap();
                let photos = guard.get_photos_for_month_year(year, month);
                photos_list_js = photos_html_helpers::get_photoswipe_js_item_list(photos, start_index, per_page);
            }
            else if let DateParams::YearOnly(year) = date_params {
                let guard = photo_results.date_accessor.read().unwrap();
                let photos = guard.get_photos_for_year(year);
                photos_list_js = photos_html_helpers::get_photoswipe_js_item_list(photos, start_index, per_page);
            }
            else {
                photos_list_js = String::new();
            }

            let wrg = WebResponseGeneratorTemplateFile::from(&combine_paths(&self.main_web_content_path, "dates_slideshow.tmpl"),
                                    &self.html_base_href)
                                    .add_field(&site_nav_header_html)
                                    .add_field(&content_html)
                                    .add_field(&photos_list_js);

            response = wrg.get_response_string();
        }
        else {
            let dates_bar_html;
            match date_params {
                // TODO: something better than this... Push DateParams into get_dates_datesbar_html()?
                DateParams::YearAndMonth { year, month } => {
                    dates_bar_html = photos_html_helpers::get_dates_datesbar_html(&photo_results, year, month, specify_date_vals_as_dirs);
                }
                DateParams::YearOnly(year) => {
                    dates_bar_html = photos_html_helpers::get_dates_datesbar_html(&photo_results, year, 14, specify_date_vals_as_dirs);
                }
                _ => {
                    dates_bar_html = photos_html_helpers::get_dates_datesbar_html(&photo_results, 0, 14, specify_date_vals_as_dirs);
                 }
            }

            let mut slideshow_url = String::new();
            let use_slideshow_url = request.get_cookie_as_uint("dates_galleryLinkToSlideshow", 1) == 1;
            if use_slideshow_url {
                let current_page_params = request.get_params_as_GET_string(false);
                if specify_date_vals_as_dirs {
                    slideshow_url = format!("dates/{}?{}&slideshow=1&", remainder_uri, current_page_params);
                }
                else {
                    slideshow_url = format!("dates/?{}&slideshow=1&", current_page_params);
                }
            }

            let content_html = photos_html_helpers::get_dates_photos_content_html(&photo_results, &date_params, request, self.lazy_photo_loading_enabled,
                                                                         &slideshow_url, specify_date_vals_as_dirs);
            
            let wrg = WebResponseGeneratorTemplateFile::from(&combine_paths(&self.main_web_content_path, "dates_gallery.tmpl"),
                                    &self.html_base_href)
                                    .add_field(&site_nav_header_html)
                                    .add_field(&dates_bar_html)
                                    .add_field(&content_html);

            response = wrg.get_response_string();
        }

        let mut stream = &connection.tpc_stream;
    
        let write_res = stream.write(response.as_bytes());
        if write_res.is_err() {
            return HandleRequestResult::RequestHandledDisconnect;
        }
        stream.flush().unwrap();

        return HandleRequestResult::RequestHandledOK;
    }

    fn handle_locations_request(&self, connection: &RequestConnection, request: &WebRequest) -> HandleRequestResult {
        let mut query_params = QueryParams::new();
    
        let want_slr = request.get_param_or_cookie_as_uint("typeSLR", "locations_typeSLR", 1) == 1;
        let want_drone = request.get_param_or_cookie_as_uint("typeDrone", "locations_typeDrone", 0) == 1;
    
        let slideshow = request.get_param_as_uint("slideshow", 0);
        let gallery = request.get_param_as_uint("gallery", 0);

        let per_page = request.get_param_as_uint("perPage", 100) as usize;
        let start_index = request.get_param_as_uint("startIndex", 0) as usize;

        let min_thumbnail_size = request.get_param_or_cookie_as_uint("thumbnailSize", "locations_thumbnailSizeValue", 500);
    
        query_params.source_types = QueryParams::make_source_type(want_slr, want_drone);
    
        let photo_results = self.photo_catalogue.query_engine.get_photo_results(&self.photo_catalogue.items, query_params,
                                     AccessorBuildFlags::BUILD_LOCATION_ACCESSOR as u32);
        
        let location_path = request.get_param("locationPath");

        let location_bar_html = photos_html_helpers::get_locations_location_bar_html(&request);

        let mut response = String::new();

        if !location_path.is_empty() && slideshow == 1 {
            let mut site_nav_header_html = photos_html_helpers::generate_main_sitenav_code(GenMainSitenavCodeParams::new(
                false, false, "locations_", &self.main_web_content_path));
            
            // add locations bar on to site name header html...
            site_nav_header_html.push_str(&format!("\n{}\n", location_bar_html));

            let guard = photo_results.location_accessor.read().unwrap();
            let photos = guard.get_photos_for_location(&location_path).unwrap();

            // TODO: is obeying the pagination stuff wanted/worth it?
		    //       can we jump to a particular photo index in some other way?

            let mut content_and_pagination_html = "<a href=\"javascript:openPhotoSwipe();\">slide show overlay</a><br><br>\n";
            if per_page > 0 {

            }

            let photos_list_js = photos_html_helpers::get_photoswipe_js_item_list(photos, start_index, per_page);
            let wrg = WebResponseGeneratorTemplateFile::from(&combine_paths(&self.main_web_content_path, "locations_slideshow.tmpl"),
                                    &self.html_base_href)
                                    .add_field(&site_nav_header_html)
                                    .add_field(&content_and_pagination_html)
                                    .add_field(&photos_list_js);

            response = wrg.get_response_string();
        }
        else if !location_path.is_empty() && gallery == 1 {
            let mut site_nav_header_html = photos_html_helpers::generate_main_sitenav_code(GenMainSitenavCodeParams::new(
                true, true, "locations_", &self.main_web_content_path));
            
            // add locations bar on to site name header html...
            site_nav_header_html.push_str(&format!("\n{}\n", location_bar_html));

            let guard = photo_results.location_accessor.read().unwrap();
            let photos = guard.get_photos_for_location(&location_path);

            let mut photos_list_html = String::new();
            let mut pagination_html = String::new();

            if let Some(photos) = photos {
                if !photos.is_empty() {
                    // normal gallery
                    if per_page > 0 {
                        let total_photos = photos.len();
                        pagination_html = photos_html_helpers::get_pagination_code("locations/", request, total_photos, start_index,
                                                                             per_page, true, true);
                    }

                    let lazy_load = self.lazy_photo_loading_enabled && request.get_param_or_cookie_as_uint("lazyLoading", 
                                                                        "locations_lazyLoading", 1) == 1;      

                    let mut slideshow_url = "".to_string();
                    if request.get_cookie_as_uint("locations_galleryLinkToSlideshow", 1) == 1 {
                        let current_page_params = request.get_params_as_GET_string(false);
                        slideshow_url = format!("locations/?{}&slideshow=1&", current_page_params);
                    }

                    photos_list_html = photos_html_helpers::get_simple_image_list_within_custom_div_tag_with_style(&photos,
                        "gallery_item", start_index, per_page, min_thumbnail_size, lazy_load, &slideshow_url);
        
                    let wrg = WebResponseGeneratorTemplateFile::from(&combine_paths(&self.main_web_content_path, "locations_gallery.tmpl"),
                                            &self.html_base_href)
                                            .add_field(&site_nav_header_html)
                                            .add_field(&photos_list_html)
                                            .add_field(&pagination_html);
        
                    response = wrg.get_response_string();
                }
            }
        }
        else {
            // otherwise, it's the main overview page with lists of locations two levels down, plus a few photos per location...
            let site_nav_header_html = photos_html_helpers::generate_main_sitenav_code(GenMainSitenavCodeParams::new(
                false, true, "locations_", &self.main_web_content_path));

 //           let guard = photo_results.location_accessor.read().unwrap();
//            let photos = guard.get_photos_for_location(&location_path);

            let content_html = photos_html_helpers::get_locations_overview_page_html(&photo_results, &request);

            let wrg = WebResponseGeneratorTemplateFile::from(&combine_paths(&self.main_web_content_path, "locations_overview.tmpl"),
                                            &self.html_base_href)
                                            .add_field(&site_nav_header_html)
                                            .add_field(&location_bar_html)
                                            .add_field(&content_html);
        
                    response = wrg.get_response_string();
        }

        let mut stream = &connection.tpc_stream;
    
        let write_res = stream.write(response.as_bytes());
        if write_res.is_err() {
            return HandleRequestResult::RequestHandledDisconnect;
        }
        stream.flush().unwrap();

        return HandleRequestResult::RequestHandledOK;
    }
}

fn get_date_params_from_request(request: &WebRequest, check_url_path: bool, remainder_uri: &str) -> DateParams {
    let mut date_params = DateParams::Invalid;

    if check_url_path && !remainder_uri.is_empty() {
        let mut first_level_directory = String::new();
        let mut remainder = String::new();
        if split_first_level_directory_and_remainder(remainder_uri, &mut first_level_directory, &mut remainder) {
            if let Ok(year) = first_level_directory.parse::<u32>() {
                // we have a year...

                // see if we also have a month...
                if let Ok(month) = remainder.parse::<u32>() {
                    // we have a remainder, so it's hopefully the 0-based month?
                    date_params = DateParams::YearAndMonth { year, month };
                }
                else {
                    // nope, just a year
                    date_params = DateParams::YearOnly(year);
                }
            }
        }
        else {
            // likely just the year
            if let Ok(year) = remainder_uri.parse::<u32>() {
                // we have a remainder, so it's hopefully the 0-based month?
                date_params = DateParams::YearOnly(year);
            }
        }
    }

    if request.has_param("year") {
        let year = request.get_param_as_uint("year", 0);

        if request.has_param("month") {
            date_params = DateParams::YearAndMonth { year, month: request.get_param_as_uint("month", 0) };
        }
        else {
            date_params = DateParams::YearOnly(year);
        }
    }

    return date_params;
}