#[macro_use] extern crate rocket;
extern crate rand;
use rocket::serde::json::{Json, Value, json};
use wasmtime::*;
use anyhow::Result;
use std::{thread, time};

use nix::unistd::{fork, ForkResult};

// static mut functionName : String = String::new(); //todo: Can't deal with the global variable 


// unsafe fn init(funcName : &str){
//     println!("Init...");
//     functionName = String::from(funcName);
//     println!("Init fininshed");
// }

fn run() -> Result<()>{
    let ten_mins = time::Duration::from_secs(5);
    thread::sleep(ten_mins);

    let engine = Engine::default();
    let module = Module::from_file(&engine, "main.wat")?;
    let mut store = Store::new(&engine, ());
    let instance = Instance::new(&mut store, &module, &[])?;
    let answer = instance.get_func(&mut store, "gcd")
        .expect("was not an exported function");
    let answer = answer.typed::<(i32,i32), i32, _>(&store)?;
    let result = answer.call(&mut store, (27,6))?;
    println!("Result: {:?}", result);
    Ok(())
}

#[get("/status")]
fn run_status() -> Value {
    // let mut rng = rand::thread_rng();
    // let x = rng.gen_range(0..10);
    // let ten_millis = time::Duration::from_secs(x);
    // thread::sleep(ten_millis);
    // json!({ "status": "ok", "times": x })
    json!({"status" : "ok"})
}

// todo : 
// 1.Pass PID outside to cGroup
// 2.Reuse the process for resource control
// 3.Use something else to alter fork()

#[get("/run")]
fn run_code() -> Value{
    // let start = Instant::now();
    // let handle = thread::spawn(||{run()});
    // handle.join().unwrap();
    // let useTime = start.elapsed().as_secs_f32();
    // json!({"last" : useTime})
    match fork(){
        Ok(ForkResult::Parent{child, ..}) => {
            println!("Proxy forks a new process with pid {}", child);
            json!({"pid" : child.as_raw()})
        }
        Ok(ForkResult::Child) => {
            let t = run();
            match t{
                Ok(_)=>{println!("Call run success");}
                Err(e)=>{println!("Call run failed : {e:?}")}
            }
            json!({"last" : 3})
        }
        Err(_) => {
            println!("fork error");
            json!({"last" : -1})
        }
    }
}

#[launch]
fn rocket() -> _ {
    rocket::build().mount("/", routes![run_status,run_code])
}