use std::{net::SocketAddr, thread, io, time::Duration, collections::HashMap};

use axum::{Router, routing::post, Json, http::StatusCode, Server, extract::State, debug_handler};
use reqwest::header::CONTENT_LENGTH;
use serde::Deserialize;
use serde_json::{Value, json};

#[derive(Deserialize)]
struct Job {
    value: u64,
    worker: i32
}
#[derive(Clone)]
struct AppState {
    client: reqwest::blocking::Client
}

#[tokio::main]
async fn main() {

    let http_client = reqwest::blocking::Client::builder().no_gzip().build().unwrap();
    
    let app = Router::new()
        .route("/", post(job_received))
        .with_state(AppState { client: http_client });

        let addr = SocketAddr::from(([0, 0, 0, 0], 3000));
        println!("worker started");
        axum::Server::bind(&addr)
            .serve(app.into_make_service())
            .await
            .unwrap();
    
}

async fn job_received(
    State(s): State<AppState>,
    Json(body): Json<Job>
) -> (StatusCode, Json<Value>) {

    let state = s.clone();
    println!("value: {}, worker: {}", body.value, body.worker);
    thread::spawn(move || {
        thread::sleep(Duration::from_secs(body.value));
        let mut json_map = HashMap::new();
        json_map.insert("worker", body.worker);
        json_map.insert("freed_qty", body.value as i32);
        
        match state.client
            .post("http://localhost:8080/done")
            .timeout(Duration::from_secs(1))
            // .header(CONTENT_LENGTH, serde_json::to_string(&json_map).unwrap().len())
            .json(&json_map)
            .send() {
                Err(e) => println!("err while requesting: {:?}", e),
                Ok(_) => println!("job done!")
            }
    });

    (StatusCode::OK, Json(json!({"success": true})))

} 