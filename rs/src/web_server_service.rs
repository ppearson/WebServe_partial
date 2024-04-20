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


use crate::configuration::Configuration;
use crate::thread_pool::ThreadPool;
use crate::web_server_common::RequestConnection;
use crate::main_request_handler::MainRequestHandler;

use std::sync::Arc;
use std::sync::RwLock;
use std::net::TcpListener;

pub struct WebServerService {
//    inner : Arc<WebServerServiceImpl>,
    inner: Arc<RwLock<WebServerServiceImpl>>,
}

// inner one
pub struct WebServerServiceImpl {
    config:             Configuration,
    request_handler:    MainRequestHandler,
}

impl WebServerService {
    pub fn init(config: &Configuration) -> WebServerService {
        let web_server_impl = WebServerServiceImpl{ config: config.clone(), request_handler: MainRequestHandler::init(config) };
        WebServerService { inner: Arc::new(RwLock::new(web_server_impl))}
    }

    pub fn run(&self) {
        let bind_address_port = format!("0.0.0.0:{}", self.inner.read().unwrap().config.port_number_http);
        println!("Binding to address: {}...", bind_address_port);

        let listener = TcpListener::bind(bind_address_port).unwrap();
        let num_worker_threads = self.inner.read().unwrap().config.worker_threads;
        println!("Using: {} worker {}.", num_worker_threads, if num_worker_threads == 1 { "thread" } else { "threads" });
        let pool = ThreadPool::new(num_worker_threads as usize);

        // Note: TcpListener has a hard-coded backlog value of 128...
        for stream in listener.incoming() {
            let request_connection = RequestConnection::create(stream.unwrap());

            // TODO: is this only increasing the ref count? hopefully...
            let local_self_arc = self.inner.clone();
            
            pool.execute(move || {
                let local_self_arc_1 = local_self_arc.read().unwrap();
                local_self_arc_1.handle_connection(request_connection);
            });
        }
    }
}

impl WebServerServiceImpl {
    fn handle_connection(&self, connection: RequestConnection) {
        self.request_handler.handle_request(connection);
    }
}
