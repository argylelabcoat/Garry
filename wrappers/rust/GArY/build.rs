use std::env;
use std::path::PathBuf;

fn main() {
    let project_root = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .parent()
        .unwrap()
        .parent()
        .unwrap()
        .to_path_buf();

    let build_dir = project_root.join("build");

    if build_dir.exists() {
        println!("cargo:rustc-link-search=native={}", build_dir.display());
    }

    let libgarry_a = build_dir.join("libgarry.a");
    let garry_lib = build_dir.join("garry.lib");

    if libgarry_a.exists() || garry_lib.exists() {
        println!("cargo:rustc-link-lib=static=garry");

        if cfg!(target_os = "macos") {
            println!("cargo:rustc-link-lib=framework=Security");
            println!("cargo:rustc-link-lib=framework=CoreFoundation");
        } else if cfg!(target_os = "linux") {
            println!("cargo:rustc-link-lib=dylib=pthread");
            println!("cargo:rustc-link-lib=dylib=m");
        } else if cfg!(target_os = "windows") {
            println!("cargo:rustc-link-lib=dylib=advapi32");
        }
    } else {
        println!("cargo:warning=libgarry not found in build/ — link manually when using FFI");
    }
}
