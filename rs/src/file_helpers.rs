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


// TODO: some of these might not be needed in theory, but are here for convenience...

pub fn remove_prefix_from_path(path: &mut String, prefix_path: &str) -> bool {
    // TOOD: can this be optimised?
    if path[0..prefix_path.len()].to_string() == *prefix_path {
        // the path begins with the prefix, so just remove it...
        let new_path = path[prefix_path.len()..].to_string();
        *path = new_path;
        return true;
    }

    return false;
}

pub fn combine_paths(path0: &str, path1: &str) -> String {
    if path0.is_empty() {
        return path1.to_string();
    }

    let mut final_path = path0.to_string();
    if !path0.ends_with('/') {
        final_path.push_str("/");
    }

    final_path.push_str(path1);

    return final_path;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_remove_prefix_from_path_1() {
        
        let mut path = "/one/two/three/".to_string();

        let res = remove_prefix_from_path(&mut path, "/one/two/");

        assert_eq!(res, true);
        assert_eq!(path, "three/");
    }

}