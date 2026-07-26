use std::env;
use std::path::PathBuf;

fn main() {
    let manifest = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let root = manifest
        .join("../../..")
        .canonicalize()
        .unwrap_or_else(|_| manifest.join("../../.."));

    let lib = env::var("PLAYBOX_LIB_DIR").unwrap_or_else(|_| {
        root.join("build/lib").to_string_lossy().into_owned()
    });
    let include = env::var("PLAYBOX_INCLUDE_DIR").unwrap_or_else(|_| {
        root.join("include").to_string_lossy().into_owned()
    });

    println!("cargo:rustc-link-search=native={lib}");
    println!("cargo:rustc-link-lib=playbox");
    println!("cargo:rustc-link-lib=m");
    println!("cargo:rerun-if-env-changed=PLAYBOX_LIB_DIR");
    println!("cargo:rerun-if-env-changed=PLAYBOX_INCLUDE_DIR");
    println!("cargo:rerun-if-changed={include}/playbox/pb.h");
}
