/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

use std::env;

fn main() {
    println!("cargo:rerun-if-env-changed=AZURE_SPHERE_TARGET_HARDWARE");
    println!(
        "cargo:rustc-cfg=azure_sphere_target_hardware={:?}",
        env::var("AZURE_SPHERE_TARGET_HARDWARE").unwrap()
    );

    println!("cargo:rerun-if-env-changed=AZURE_SPHERE_TARGET_DEFINITION");
    println!(
        "cargo:rustc-cfg=azure_sphere_target_definition={:?}",
        env::var("AZURE_SPHERE_TARGET_DEFINITION").unwrap()
    );
}
