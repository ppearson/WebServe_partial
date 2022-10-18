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
use super::photo_item::{PhotoItem};
use std::fs;

use crate::file_helpers::combine_paths;
use crate::string_helpers::{self, simple_encode_string};
use crate::web_request::WebRequest;
use crate::photos_handler::photos_common::{DateParams};
use crate::photos_handler::photo_results::{PhotoResults};

pub struct GenMainSitenavCodeParams {
    add_play_slideshow_icon:            bool,
    add_view_settings_icon:             bool,

    view_settings_template_prefix:      String,
    main_web_content_path:              String,
}

impl GenMainSitenavCodeParams {
    pub fn new(add_play_slideshow_icon: bool, add_view_settings_icon: bool, vs_prefix: &str,
                main_web_content_path: &str) -> GenMainSitenavCodeParams {
        GenMainSitenavCodeParams { add_play_slideshow_icon, add_view_settings_icon,
            view_settings_template_prefix: vs_prefix.to_string(), main_web_content_path: main_web_content_path.to_string()}
    }

}

pub fn generate_main_sitenav_code(params: GenMainSitenavCodeParams) -> String {
    let mut final_html = "<div class=\"topnav\">\n\
    <a href=\"\">Home</a>\n\
    <a href=\"photostream/\">Photostream</a>\n\
    <a href=\"dates/\">Dates</a>\n\
    <a href=\"locations/\">Locations</a>\n\
    <a href=\"sets/\">Sets</a>\n".to_string();

    if params.add_play_slideshow_icon || params.add_view_settings_icon {
        final_html.push_str("<div class=\"subbarRight\">\n");

        if params.add_play_slideshow_icon {
            final_html.push_str("<img src=\"icons/play_main_navbar.svg\" onclick=\"redirectToMainSlideshow()\" style=\"float:right;\">\n");
        }

        if params.add_view_settings_icon {
            final_html.push_str(&format!(r#"<img src="icons/settings_main_navbar.svg" onclick="toggleViewSettingsPopupPanel('{}')" style="float:right;">"#,
                                                            &params.view_settings_template_prefix));
            final_html.push('\n');

            let path = format!("{}view_settings_popup_div.stmpl", params.view_settings_template_prefix);
            final_html.push_str(&load_template_snippit(&combine_paths(&params.main_web_content_path, &path)));
        }

        final_html.push_str("</div>\n");
    }

    if params.add_view_settings_icon {
        // somewhat distasteful, but for positioning it needs to be near the button invoking the popup...
        let path = format!("{}view_settings_popup_scripts.stmpl", params.view_settings_template_prefix);
        final_html.push_str(&format!("\n{}\n", load_template_snippit(&combine_paths(&params.main_web_content_path, &path))));
    }

    final_html.push_str("</div>\n");

    return final_html;
}

fn load_template_snippit(filename: &str) -> String {

    // TODO: error handling...
    let res = fs::read_to_string(filename);
    if res.is_ok() {
        return res.unwrap();
    }
    else {
        return "".to_string();
    }
}

pub fn get_pagination_code(url: &str, web_request: &WebRequest, total_count: usize, start_index: usize,
                            per_page: usize, add_first_and_last: bool, add_to_existing_get_params: bool) -> String {
    if per_page == 0 {
        return "".to_string();
    }

    let mut final_html = "<div class=\"pagination\">\n".to_string();
    let mut num_pages = total_count / per_page;
    num_pages += (total_count % per_page > 0) as usize;

    if num_pages <= 1 {
        return "".to_string();
    }

    const NUM_TOTAL_PAGES_TO_SHOW: usize = 9;
    const NUM_BOOKEND_PAGES_TO_SHOW: usize = 4; // pages either side (excluding first and last buttons)

    let max_pages = NUM_TOTAL_PAGES_TO_SHOW;

    let mut first_page = 0;

    let mut page_items_to_show = std::cmp::min(max_pages, num_pages);
    let current_page_index = start_index / per_page;

    if num_pages > max_pages {
        // if we've got more pages than the limit to paginate, and our current page
        // index is greater than that, we need to offset so the current page is roughly
        // in the centre of a windowed range

        if current_page_index > NUM_BOOKEND_PAGES_TO_SHOW {
            first_page = current_page_index - NUM_BOOKEND_PAGES_TO_SHOW;
        }

        page_items_to_show = std::cmp::min(num_pages - first_page, max_pages);
    }

    let mut existing_get_params_string = String::new();
    if add_to_existing_get_params {
        existing_get_params_string = web_request.get_params_as_GET_string(true);
    }

    if add_first_and_last && first_page > 0 && current_page_index >= NUM_BOOKEND_PAGES_TO_SHOW {
        final_html.push_str(&format!(" <a href=\"{}?{}&startIndex=0&perPage={}\">«&nbsp;First</a>\n",
                            &url, &existing_get_params_string, per_page));
    }

    let end_page = first_page + page_items_to_show;
    for i in first_page..end_page {
        let link_start_index = i * per_page;
        if i == current_page_index {
            final_html.push_str(&format!("  <a href=\"{}?{}&startIndex={}&perPage={}\" class=\"active\">{}</a>\n",
                                &url, &existing_get_params_string, link_start_index, per_page, i + 1));
        }
        else {
            final_html.push_str(&format!("  <a href=\"{}?{}&startIndex={}&perPage={}\">{}</a>\n",
                                &url, &existing_get_params_string, link_start_index, per_page, i + 1));
        }
    }

    if add_first_and_last {
        let last_page_start_index = (num_pages - 1) * per_page;
        final_html.push_str(&format!("  <a href=\"{}?{}&startIndex={}&perPage={}\">Last&nbsp;»</a>\n",
                            &url, &existing_get_params_string, last_page_start_index, per_page));
    }

    final_html.push_str("</div>\n");

    return final_html;
}

pub fn get_simple_image_list_within_custom_div_tag_with_style(photos: &Vec<Arc<Box<PhotoItem>>>, div_tag: &str, start_index: usize,
                                                              per_page: usize, min_thumbnail_size: u32, lazy_load: bool,
                                                              slide_show_url: &str) -> String {
    
    let mut final_html = String::new();

    if start_index as usize >= photos.len() {
        return final_html;
    }

    let main_width = min_thumbnail_size as f32 / 2.0;

    let mut end_index = photos.len() - 1;
    if per_page > 0 && start_index != end_index {
        end_index = std::cmp::min(end_index, start_index + per_page);
    }

    let use_slideshow_url = !slide_show_url.is_empty();

    let mut index = 0; // needs to start at 0, not start_index as it's an index into current page...

    for photo in &photos[start_index..=end_index] {

        let mut thumbnail_repr = photo.representations.get_smallest_representation_matching_criteria_min_dimension(min_thumbnail_size).unwrap();

        let aspect_ratio = thumbnail_repr.aspect_ratio;
        // try and bodge very wide images, so that we get higher res ones for those
        if aspect_ratio > 2.2f32 {
            let thumbnail_repr_bigger = photo.representations.get_smallest_representation_matching_criteria_min_dimension(min_thumbnail_size + 200);
            if thumbnail_repr_bigger.is_some() {
                thumbnail_repr = thumbnail_repr_bigger.unwrap();
            }
        }

        let style_string = format!("flex-basis: {}px; flex-grow: {};", (main_width * aspect_ratio) as u32, aspect_ratio);

        final_html.push_str(&format!("<div class=\"{}\" style=\"{}\">\n", div_tag, &style_string));

        let large_rep = photo.representations.get_first_representation_matching_criteria_min_dimension(1000, true);
        if large_rep.is_some() {
            if use_slideshow_url {
                final_html.push_str(&format!(r#" <a target="_blank" href="{}gotoIndex={}">"#, slide_show_url, index));
                final_html.push('\n');
            }
        }

        if lazy_load {
            final_html.push_str(&format!(" <img data-src=\"{}\" class=\"lazyload\"/>\n", &thumbnail_repr.relative_file_path));
        }
        else {
            final_html.push_str(&format!(" <img src=\"{}\"/>\n", &thumbnail_repr.relative_file_path));
        }

        if large_rep.is_some() {
            final_html.push_str(" </a>\n");
        }

        final_html.push_str("</div>\n");

        index += 1;
    }

    return final_html;
}

pub fn get_photoswipe_js_item_list(photos: &Vec<Arc<Box<PhotoItem>>>, start_index: usize, per_page: usize) -> String {
    let mut final_js = "var items = [\n".to_string();

    let mut end_index = photos.len() - 1;
    if per_page > 0 && start_index != end_index {
        end_index = std::cmp::min(end_index, start_index + per_page);
    }

    let num_items = end_index - start_index;
    let mut count: usize = 0;

    for photo in &photos[start_index..=end_index] {
        let repre = photo.representations.get_first_representation_matching_criteria_min_dimension(100, true).unwrap();

        let image_path = &repre.relative_file_path;

        // TODO: work out why this needs to be - 0 and not - 1 to do the correct thing.... ?
        if count != (num_items - 0) {
            // if we're not the last one, we need a comma on the end...
            final_js.push_str(&format!("\t{{\n\t\tsrc: '{}',\n\t\tw: {},\n\t\th: {}\n\t}},\n", image_path, repre.width, repre.height));
        }
        else {
            // otherwise, we don't... TODO: could combine these?
            final_js.push_str(&format!("\t{{\n\t\tsrc: '{}',\n\t\tw: {},\n\t\th: {}\n\t}}\n", image_path, repre.width, repre.height));
        }

        count += 1;
    }

    final_js.push_str("\t];\n");

    return final_js;
}

const MONTH_NAMES: &[&str] = &["January", "February", "March", "April", "May", "June",
                                "July", "August", "September", "October", "November", "December"];

pub fn get_dates_datesbar_html(photos: &PhotoResults, active_year: u32, active_month: u32, use_uri_for_components: bool) -> String {
    let mut final_html = String::new();

    let guard = photos.date_accessor.read().unwrap();

    let years = guard.get_list_of_years();

    for year in years {
        if year == active_year {
            final_html.push_str("<button class=\"dropdown-btn active\">");
        }
        else {
            final_html.push_str("<button class=\"dropdown-btn\">");
        }
        final_html.push_str(&format!("{}</button>\n", year));

        // set the active year item to be expanded...
        if year == active_year {
            final_html.push_str("<div class=\"dropdown-container\" style=\"display: block;\">\n");
        }
        else {
            final_html.push_str("<div class=\"dropdown-container\">\n");
        }

        // TODO: work out how to get the above collapsable item to be a link as well
        if use_uri_for_components {
            final_html.push_str(&format!("  <a href=\"dates/{}\">(all)</a>\n", year));
        }
        else {
            final_html.push_str(&format!("  <a href=\"dates/?year={}\">(all)</a>\n", year));
        }

        let months = guard.get_list_of_months_for_year(year);
        for month in months {
            if year == active_year && *month == active_month {
                final_html.push_str(" <div class=\"activeMonth\">\n");
            }

            if use_uri_for_components {
                final_html.push_str(&format!("  <a href=\"dates/{}/{}\">{}</a>\n", year, month, MONTH_NAMES[*month as usize]));
            }
            else {
                final_html.push_str(&format!("  <a href=\"dates/?year={}&month={}\">{}</a>\n", year, month, MONTH_NAMES[*month as usize]));
            }

            if year == active_year && *month == active_month {
                final_html.push_str(" </div>\n");
            }
        }

        final_html.push_str(" </div>\n");
    }

    return final_html;
}

pub fn get_dates_photos_content_html(photo_results: &PhotoResults, date_params: &DateParams, request: &WebRequest, overall_lazy_loading: bool,
                                     slideshow_url: &str, use_uri_for_components: bool) -> String {
    let mut final_html = String::new();

    if *date_params == DateParams::Invalid {
        return final_html;
    }

    let thumbnail_size = request.get_param_or_cookie_as_uint("thumbnailSize", "dates_thumbnailSizeValue", 500);
    let main_width = thumbnail_size as f32 / 2.0;

    let lazy_load = overall_lazy_loading && request.get_param_or_cookie_as_uint("lazyLoading", "dates_lazyLoading", 1) == 1;

    let mut index = 0u32; // index for slideshow goto link

    let use_slideshow_url = !slideshow_url.is_empty();

    if let DateParams::YearAndMonth{year, month} = date_params {
        final_html.push_str("<div class=\"gallery\">\n");

        let guard = photo_results.date_accessor.read().unwrap();

        let photos = guard.get_photos_for_month_year(*year, *month);

        for photo_item in photos {
            let thumbnail_repr = photo_item.representations.get_smallest_representation_matching_criteria_min_dimension(thumbnail_size);
            let mut thumbnail_repr = thumbnail_repr.unwrap();

            // try and bodge very wide images, so that we get higher res ones for those
            if thumbnail_repr.aspect_ratio > 2.2 {
                let thumbnail_repr_bigger = photo_item.representations.get_smallest_representation_matching_criteria_min_dimension(thumbnail_size + 200);
                thumbnail_repr = thumbnail_repr_bigger.unwrap();
            }

            let style_string = format!("flex-basis: {}px; flex-grow: {};", (main_width * thumbnail_repr.aspect_ratio as f32) as u32,
                                                                                thumbnail_repr.aspect_ratio);

            final_html.push_str(&format!(r#"<div class="gallery_item" style="{}\">"#, style_string));
            final_html.push('\n');

            let large_repr = photo_item.representations.get_first_representation_matching_criteria_min_dimension(1000, true).unwrap();
            if use_slideshow_url {
                final_html.push_str(&format!(r#" <a target="_blank" href="{}gotoIndex={}"#, slideshow_url, index));
                final_html.push_str("\">\n");
            }
            else {
                final_html.push_str(&format!(r#" <a target="_blank" href="{}"#, large_repr.relative_file_path));
                final_html.push_str("\">\n");
            }

            if lazy_load {
                final_html.push_str(&format!(" <img data-src=\"{}\" class=\"lazyload\"/>\n", thumbnail_repr.relative_file_path));
            }
            else {
                final_html.push_str(&format!(" <img data-src=\"{}\"/>\n", thumbnail_repr.relative_file_path));
            }

            // if large_repr
            final_html.push_str(" </a>\n");

            index += 1;

            final_html.push_str("</div>\n");
        }

        final_html.push_str("</div>\n");
    }
    else if let DateParams::YearOnly(year) = date_params {
        // list all the months
        let guard = photo_results.date_accessor.read().unwrap();
        let months_for_year = guard.get_list_of_months_for_year(*year);
        for month_index in months_for_year {
            // print month heading
            final_html.push_str(&format!("<h3>{}</h3>\n", MONTH_NAMES[*month_index as usize]));

            // link to slideshow version for the month/year
            if use_uri_for_components {
                final_html.push_str(&format!("<a href=\"dates/{}/{}?slideshow=1\"><img src=\"icons/play_main_navbar.svg\" style=\"float:right;\"></a><br><br>\n",
                                        year, *month_index));
            }
            else {
                final_html.push_str(&format!("<a href=\"dates/?year={}&month={}&slideshow=1\"><img src=\"icons/play_main_navbar.svg\" style=\"float:right;\"></a><br><br>\n",
                                        year, *month_index));
            }

            // now the gallery of photos for this month
            final_html.push_str("<div class=\"gallery\">\n");

            // TODO: this is duplication of the YearAndMonth branch above, we could re-use code...
            let photos = guard.get_photos_for_month_year(*year, *month_index);

            for photo_item in photos {
                let thumbnail_repr = photo_item.representations.get_smallest_representation_matching_criteria_min_dimension(thumbnail_size);
                let mut thumbnail_repr = thumbnail_repr.unwrap();

                // try and bodge very wide images, so that we get higher res ones for those
                if thumbnail_repr.aspect_ratio > 2.2 {
                    let thumbnail_repr_bigger = photo_item.representations.get_smallest_representation_matching_criteria_min_dimension(thumbnail_size + 200);
                    thumbnail_repr = thumbnail_repr_bigger.unwrap();
                }

                let style_string = format!("flex-basis: {}px; flex-grow: {};", (main_width * thumbnail_repr.aspect_ratio as f32) as u32,
                                                                                    thumbnail_repr.aspect_ratio);

                final_html.push_str(&format!(r#"<div class="gallery_item" style="{}\">"#, style_string));
                final_html.push('\n');

                let large_repr = photo_item.representations.get_first_representation_matching_criteria_min_dimension(1000, true).unwrap();
                if use_slideshow_url {
                    final_html.push_str(&format!(r#" <a target="_blank" href="{}gotoIndex={}"#, slideshow_url, index));
                    final_html.push_str("\">\n");
                }
                else {
                    final_html.push_str(&format!(r#" <a target="_blank" href="{}"#, large_repr.relative_file_path));
                    final_html.push_str("\">\n");
                }

                if lazy_load {
                    final_html.push_str(&format!(" <img data-src=\"{}\" class=\"lazyload\"/>\n", thumbnail_repr.relative_file_path));
                }
                else {
                    final_html.push_str(&format!(" <img data-src=\"{}\"/>\n", thumbnail_repr.relative_file_path));
                }

                // if large_repr
                final_html.push_str(" </a>\n");

                index += 1;

                final_html.push_str("</div>\n");
            }
            final_html.push_str("</div>\n");
        }
    }
    else {

    }

    return final_html;
}

// could just have passed through the locationPath value directly, but we might want to do something a bit different
// in the future, so...
pub fn get_locations_location_bar_html(request: &WebRequest) -> String {
    let current_location_path = request.get_param("locationPath");
    if current_location_path.is_empty() {
        return "".to_string();
    }

    let location_components: Vec<&str> = current_location_path.split('/').map(|x| x.trim()).collect();

    let mut final_html = "<div class=\"subbar\">\n".to_string();
    final_html.push_str("<div class=\"subbarLeft\">\n");
    final_html.push_str("<div class=\"breadcrumb\">\n");
    final_html.push_str(" <a href=\"locations\">All</a>\n");

    let mut full_path = String::new();
    for comp_str in location_components {
        full_path = combine_paths(&full_path, comp_str);
        let encoded_location_path = simple_encode_string(&full_path);
        final_html.push_str(&format!(" <a href=\"locations?locationPath={}\">{}</a>\n", encoded_location_path, comp_str));
    }

    final_html.push_str("</div></div></div>\n");
    return final_html;
}

pub fn get_locations_overview_page_html(photo_results: &PhotoResults, request: &WebRequest) -> String {
    let mut final_html = String::new();

    let location_path = request.get_param("locationPath");
    let guard = photo_results.location_accessor.read().unwrap();
    let sub_location_names = guard.get_sub_locations_for_location(&location_path);

    if !sub_location_names.is_empty() {
        // we have further sub-locations, so list these locations and their sub-locations...
        for sub_name in sub_location_names {
            let full_sub_path = combine_paths(&location_path, &sub_name);
            let encoded_location_path = simple_encode_string(&full_sub_path);

            let sub_sub_locations = guard.get_sub_locations_for_location(&full_sub_path);

            final_html.push_str("<div class=\"locationPanel\">\n");

            // only add an actual link if there are sub-locations...
            if !sub_sub_locations.is_empty() {
                final_html.push_str(&format!("<div class=\"locationPanel-header\"><a href=\"locations?locationPath={}\">{}</a></div>\n",
                                encoded_location_path, sub_name));
            }
            else {
                // otherwise, just the title
                final_html.push_str(&format!("<div class=\"locationPanel-header\">{}</a></div>\n", sub_name));
            }

            final_html.push_str(&format!("<div class=\"locationPanel-body\"><a href=\"locations?locationPath={}&gallery=1\">View all photos</a></div>\n",
                        encoded_location_path));
            final_html.push_str("<div class=\"locationPanel-footer\">\n");

            for sub_sub_name in sub_sub_locations {
                let encoded_sub_location_path = string_helpers::simple_encode_string(&format!("{}/{}", full_sub_path, sub_sub_name));
                final_html.push_str("<div class=\"subLocationChip\">\n");
                final_html.push_str(&format!(" <a href=\"locations?locationPath={}&gallery=1\"><img src=\"icons/photo_stack.png\"></a>\n", encoded_sub_location_path));

                final_html.push_str(&format!(" <a href=\"locations?locationPath={}\">{}</a>\n", encoded_sub_location_path, sub_sub_name));
                final_html.push_str("</div>\n");
            }

            final_html.push_str("</div>\n"); // end footer
            final_html.push_str("</div>\n");
        }
    }
    else {
        // no further sub-locations, so just display photos
        // TODO: display some photos...
    }

    return final_html;
}
