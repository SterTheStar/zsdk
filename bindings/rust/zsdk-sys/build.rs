use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=../../include/zsdk/c");

    if env::var("ZSDK_LINK_ZSDK").as_deref() == Ok("1") {
        if let Ok(dir) = env::var("ZSDK_LIB_DIR") {
            println!("cargo:rustc-link-search=native={}", dir);
        } else {
            let manifest = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
            let root = manifest.parent().unwrap().parent().unwrap().parent().unwrap();
            let target = env::var("TARGET").unwrap_or_default();
            let build_dir = if target.contains("arm") {
                root.join("build_zeebo")
            } else {
                root.join("build_windows")
            };
            println!("cargo:rustc-link-search=native={}", build_dir.display());
        }
        println!("cargo:rustc-link-lib=static=zsdk");
        if env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("windows") {
            for lib in ["user32", "gdi32", "opengl32", "winmm", "ws2_32", "advapi32", "shell32"] {
                println!("cargo:rustc-link-lib=dylib={}", lib);
            }
        }
    }
}
