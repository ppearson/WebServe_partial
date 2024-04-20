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


// TODO: maybe just return a tuple in an Option?
pub fn split_first_level_directory_and_remainder(url: &str, directory: &mut String, remainder: &mut String) -> bool {

    let split_pos = url.find('/');
    if split_pos.is_none() {
        return false;
    }

    let dir_part = &url[0..split_pos.unwrap()];
    let remainder_part = &url[split_pos.unwrap()+1..];

    *directory = dir_part.to_string();
    *remainder = remainder_part.to_string();

    true
}




#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test1() {
        
        let mut dir = String::new();
        let mut remainder = String::new();

        let ret_res = split_first_level_directory_and_remainder("test/othertest", &mut dir, &mut remainder);

        assert_eq!(ret_res, true);
        assert_eq!(dir, "test");
        assert_eq!(remainder, "othertest");
    }


}