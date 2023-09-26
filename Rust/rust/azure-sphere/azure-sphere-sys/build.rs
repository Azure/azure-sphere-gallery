/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

use std::env;
use std::fs;
use std::fs::File;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
extern crate cc;

fn replace_string(source: &mut [u8], from: &str, to: &str) -> std::io::Result<()> {
    let from = from.as_bytes();
    let to = to.as_bytes();

    let mut found = false;
    for i in 0..source.len() {
        let s = &mut source[i..];
        if s.starts_with(from) {
            s[..from.len()].clone_from_slice(to);
            found = true;
        }
    }

    if found {
        Ok(())
    } else {
        Err(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            "Patch pattern not found",
        ))
    }
}

fn patch_lib(from: &Path, to: &Path) -> std::io::Result<()> {
    println!("File: {:?}", from);
    let mut file = File::open(from)?;
    let mut buffer = Vec::new();
    file.read_to_end(&mut buffer)?;

    replace_string(
        &mut buffer,
        "__aeabi_unwind_cpp_pr0",
        "__aeabi_unwind_bbb_pr0",
    )?;

    let mut new_file = File::create(to)?;
    new_file.write_all(&buffer)?;

    Ok(())
}

fn main() {
    let arv = env::var("AZURE_SPHERE_ARV").unwrap();

    // Tell cargo to invalidate the built crate whenever the wrapper changes
    println!("cargo:rerun-if-changed=bindings");

    let sdk_path = PathBuf::from(std::env::var("AzureSphereDefaultSDKDir").unwrap());

    let library_path = Path::new("../azure-sphere-sys/bindings/applibs");
    let sysroot = sdk_path.join("Sysroots").join(&arv);

    let handles = [
        "applibs/adc.h",
        "applibs/application.h",
        "applibs/applications.h",
        "applibs/certstore.h",
        "applibs/eventloop.h",
        "applibs/gpio.h",
        "applibs/i2c.h",
        "applibs/log.h",
        "applibs/networking.h",
        // empty outside of an inline function, so not needed:  "applibs/networking_curl.h",
        "applibs/powermanagement.h",
        "applibs/pwm.h",
        "applibs/rtc.h",
        "applibs/spi.h",
        "applibs/storage.h",
        "applibs/sysevent.h",
        "applibs/uart.h",
        "applibs/wificonfig.h",
        "tlsutils/deviceauth.h",
        "tlsutils/deviceauth_curl.h",
    ]
    .iter()
    .map(|header| {
        let full_header = sysroot.join("usr/include/").join(header);
        std::thread::spawn(move || {
            generate_bindings(full_header);
        })
    })
    .collect::<Vec<_>>();
    handles.into_iter().for_each(|h| h.join().unwrap());
    generate_bindings("bindings/applibs/static_inline_helpers.h");

    // Tell Cargo that if the given file changes, to rerun this build script.
    println!("cargo:rerun-if-changed=../azure-sphere-sys/static_inline_helpers.c");
    // Use the `cc` crate to build a C file and statically link it.
    let compiler_path = sysroot.join("tools/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-musleabi/arm-poky-linux-musleabi-gcc");

    cc::Build::new()
        .file("../azure-sphere-sys/static_inline_helpers.c")
        .include(library_path)
        .compiler(compiler_path)
        .flag("--sysroot")
        .flag(sysroot.to_str().unwrap())
        .flag("-fno-strict-aliasing")
        .flag("-fno-exceptions")
        .flag("-fstack-protector-strong")
        .flag("-MD")
        .flag("-march=armv7ve")
        .flag("-mthumb")
        .flag("-mfpu=neon-vfpv4")
        .flag("-mfloat-abi=hard")
        .flag("-mcpu=cortex-a7")
        .target("arm-poky-linux-musleabi")
        .compile("static_inline_helpers");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let patched_libs_path = out_path.join("patched_libs");
    let sysroot_path: PathBuf = [
        env::var("AzureSphereDefaultSDKDir").unwrap(),
        "Sysroots".to_string(),
        arv,
    ]
    .iter()
    .collect();

    let libapplibs = sysroot_path.join("usr/lib/libapplibs.so");
    println!("cargo:rerun-if-changed={:?}", libapplibs);
    fs::create_dir_all(&patched_libs_path).unwrap();
    let patched_libapplibs = patched_libs_path.join("libapplibs.so");
    patch_lib(libapplibs.as_path(), patched_libapplibs.as_path()).unwrap();

    let libc = sysroot_path.join("usr/lib/libc.so");
    println!("cargo:rerun-if-changed={:?}", libc);
    fs::create_dir_all(&patched_libs_path).unwrap();
    let patched_libc = patched_libs_path.join("libc.so");
    patch_lib(libc.as_path(), patched_libc.as_path()).unwrap();

    //println!("cargo:rustc-link-lib=dylib:c");
    //println!("cargo:rustc-link-lib=dylib:applibs");
    //println!("cargo:rustc-link-lib=dylib:tlsutils");
    println!(
        "cargo:rustc-link-search={}",
        patched_libs_path.as_path().to_str().unwrap()
    );
}

fn generate_bindings<P: AsRef<Path>>(header: P) {
    let header = header.as_ref();
    let sdk_path = PathBuf::from(std::env::var("AzureSphereDefaultSDKDir").unwrap());
    let arv = env::var("AZURE_SPHERE_ARV").unwrap();
    let sysroot = sdk_path.join("Sysroots").join(arv);
    let bindings = bindgen::Builder::default()
        .header(
            header
                .to_str()
                .expect("Only utf-8 header paths are supported"),
        )
        // Build for no_std
        .use_core()
        // use `libc` crate for ctypes
        .ctypes_prefix("libc")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        // Add the bindings folder, so .h files can reference each other without being converted to relative references.
        .clang_arg("--include-directory=bindings")
        .clang_arg("--sysroot")
        .clang_arg(sysroot.to_str().unwrap())
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(std::env::var("OUT_DIR").unwrap());
    let mut bindings_file = header
        .file_stem()
        .expect("Header file must have a file name")
        .to_owned();
    bindings_file.push(".rs");

    bindings
        .write_to_file(out_path.join(bindings_file))
        .expect("Couldn't write bindings!");
}
