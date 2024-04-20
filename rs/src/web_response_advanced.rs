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

use std::path::Path;
use std::io::Read;

use std::io::prelude::*;

use crate::web_server_common::RequestConnection;

pub trait SendAdvancedResponse {
    fn send_response(&self, connection: &RequestConnection) -> bool;
}

pub struct WebResponseAdvancedBinaryFile {
    // doesn't really seem worth it having state, but...
    file:   String,
}

impl WebResponseAdvancedBinaryFile {
    pub fn new(file: &str) -> WebResponseAdvancedBinaryFile {
        WebResponseAdvancedBinaryFile { file: file.to_string() }
    }
}

impl SendAdvancedResponse for WebResponseAdvancedBinaryFile {
    fn send_response(&self, connection: &RequestConnection) -> bool {

        if !std::path::Path::new(&self.file).exists() {
            return false;
        }

        // TODO: can we optimise some of this...?

        let extension = Path::new(&self.file).extension().unwrap();
        let extension_lower = extension.to_ascii_lowercase();
        let file_extension = extension_lower.to_str().unwrap();

        let content_type = match file_extension {
            "jpg" | "jpeg"  => "image/jpg",
            "png"           => "image/png",
            "gif"           => "image/gif",
            "bmp"           => "image/bmp",
            _               => "unknown...",
        };

        let file_size = std::fs::metadata(&self.file).unwrap().len();

        let header_response = format!(
            "HTTP/1.1 {}\r\nContent-Type: {}\r\nContent-Length: {}\r\n\r\n",
            200,
            content_type,
            file_size
        );

        let mut stream = &connection.tpc_stream;
        
        let write_res = stream.write(header_response.as_bytes());
        if write_res.is_err() {
//            let err = write_res.unwrap_err();
//            eprintln!("Error sending header: {}", err);

            // socket was likely closed by the client due to timeout...

            return false;
        }

        // TODO: chucked large files...

        const BUFFER_SIZE: usize = 16 * 1024;

        let mut file = std::fs::File::open(&self.file).unwrap();

        let mut buffer = Vec::with_capacity(BUFFER_SIZE);
        loop {
            let bytes_read = std::io::Read::by_ref(&mut file).take(BUFFER_SIZE as u64).read_to_end(&mut buffer).unwrap();

            if bytes_read == 0 {
                break;
            }

            let write_res = stream.write(&buffer[0..bytes_read]);
            if write_res.is_ok() {
                let bytes_written = write_res.unwrap();
                assert!(bytes_written == bytes_read);

                if bytes_read < BUFFER_SIZE {
                    break;
                }
                
                // buffer is extended each time read_to_end() is called, so we need this.
                // In theory, it should be very cheap, as it doesn't de-allocate the memory...
                buffer.clear();
            }
            else {
//                let err = write_res.unwrap_err();
//                eprintln!("Error sending: {}", err);

                // socket was likely closed by the client due to timeout...

                return false;
            }
        }

        stream.flush().unwrap();

        true
    }
}
