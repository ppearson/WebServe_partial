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
use std::fs;
use std::io::Read;
use std::io::{BufRead, BufReader};

pub trait GetResponseString {
    fn get_response_string(&self) -> String;
}

pub struct WebResponseGeneratorText {
    return_code:        i32,
    text:               String,
}

impl WebResponseGeneratorText {
    pub fn new(return_code: i32, text: &str) -> WebResponseGeneratorText {
        WebResponseGeneratorText { return_code, text: text.to_string() }
    }
}

impl GetResponseString for WebResponseGeneratorText {
    fn get_response_string(&self) -> String {
        let response = format!(
            "HTTP/1.1 {}\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: {}\r\n\r\n{}",
            self.return_code,
            self.text.len(),
            self.text
        );

        return response;
    }
}

pub struct WebResponseGeneratorFile {
    file:       String,
}

impl WebResponseGeneratorFile {
    pub fn new(file: String) -> WebResponseGeneratorFile {
        WebResponseGeneratorFile { file }
    }
}

impl GetResponseString for WebResponseGeneratorFile {
    fn get_response_string(&self) -> String {

        let contents;
        let return_code;

        let mut content_type = "text/html; charset=UTF-8";

        if !std::path::Path::new(&self.file).exists() {
            contents = "File not found.".to_string();
            return_code = 404;
        }
        else {
            // TODO: can this be optimised?
            let extension = Path::new(&self.file).extension().unwrap();
            let extension_lower = extension.to_ascii_lowercase();
            let file_extension = extension_lower.to_str().unwrap();

            return_code = 200;

            let mut binary = false;

            match file_extension {
                "png" => {
                    binary = true;
                    content_type = "image/png";
                }
                "jpg" => {
                    binary = true;
                    content_type = "image/jpg";
                }
                "svg" => {
                    content_type = "image/svg+xml";
                }
                "css" => {
                    content_type = "text/css; charset=UTF-8";
                }
                "js" => {
                    content_type = "application/javascript";
                }
                _ => {

                }
            }

            if binary {
                let mut buffer = Vec::new();
                let mut file = std::fs::File::open(&self.file).unwrap();

                file.read_to_end(&mut buffer).unwrap();

                // until we get full binary chucked working properly
                contents = unsafe { String::from_utf8_unchecked(buffer).to_string() } ;
            }
            else {
                contents = fs::read_to_string(&self.file).unwrap();
            }
        }

        let response = format!(
            "HTTP/1.1 {}\r\nContent-Type: {}\r\nContent-Length: {}\r\n\r\n{}",
            return_code,
            content_type,
            contents.len(),
            contents
        );

        return response;
    }
}


pub struct WebResponseGeneratorTemplateFile {
    template_args:          u32,
    path:                   String,
    content:                Vec<String>
}

impl WebResponseGeneratorTemplateFile {
    pub fn from(path: &str, content: &str) -> WebResponseGeneratorTemplateFile {
        let mut wrg = WebResponseGeneratorTemplateFile { template_args: 1, path: path.to_string(),
                                            content: Vec::with_capacity(1) };
        wrg.content.push(content.to_string());
        return wrg;
    }

    // instead of using a full Builder external class, just do this...
    pub fn add_field(mut self, content: &str) -> Self {
        self.content.push(content.to_string());
        self.template_args += 1;
        return self;
    }
}

impl GetResponseString for WebResponseGeneratorTemplateFile {
    fn get_response_string(&self) -> String {

        let file = std::fs::File::open(&self.path).unwrap();
        let reader = BufReader::new(file);
        // TODO: error handling...

        let mut num_matches = 0;

        let mut contents = String::new();

        for line in reader.lines() {
            let line = line.unwrap();

            if self.template_args == 1 {
                if num_matches < 1 {
                    if let Some(pos) = line.find("<%%>") {
                        let mut templated_line = line.clone();
                        templated_line.replace_range(pos..pos+4, &self.content[num_matches as usize]);
                        templated_line.push_str("\n");
    
                        contents.push_str(&templated_line);
                        num_matches += 1;
                    }
                    else {
                        // just add the line...
                        contents.push_str(&line);
                        contents.push_str("\n");
                    }
                }
                else {
                    // just add the line...
                    contents.push_str(&line);
                    contents.push_str("\n");
                }
            }
            else {
                if num_matches < self.template_args {
                    let find_string = format!("<%{}%>", num_matches + 1);
                    if let Some(pos) = line.find(&find_string) {
                        let mut templated_line = line.clone();
                        templated_line.replace_range(pos..pos+5, &self.content[num_matches as usize]);
                        templated_line.push_str("\n");
    
                        contents.push_str(&templated_line);
                        num_matches += 1;
                    }
                    else {
                        // just add the line...
                        contents.push_str(&line);
                        contents.push_str("\n");
                    }
                }
                else {
                    // just add the line...
                    contents.push_str(&line);
                    contents.push_str("\n");
                }
            }
        }

        let response = format!(
            "HTTP/1.1 200\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: {}\r\n\r\n{}",
            contents.len(),
            contents
        );

        return response;
    }
}



#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_WebResponseGeneratorText1() {
        
        let resp_gen = WebResponseGeneratorText::new(200, "The quick brown dog jumped over the lazy fox.");
        let resp_string = resp_gen.get_response_string();

        let expected_response_string = 
        "HTTP/1.1 200\r\n\
         Content-Type: text/html; charset=UTF-8\r\n\
         Content-Length: 45\r\n\r\n\
         The quick brown dog jumped over the lazy fox.";
       
        assert_eq!(resp_string, expected_response_string);
    }


}
