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


pub struct RequestConnection {
pub tpc_stream:         std::net::TcpStream,
}

impl RequestConnection {

    pub fn create(tpc_stream: std::net::TcpStream) -> RequestConnection {
        RequestConnection { tpc_stream: tpc_stream }
    }
}