use std::path::{Path, PathBuf};

#[cfg(target_os = "macos")]
pub const MACOS_SDK_VERSION: &str = "1.3.268.1";

#[cfg(target_os = "macos")]
pub(crate) fn download_prebuilt_molten<P: AsRef<Path>>(target_dir: &P) {
    use std::process::Command;

    std::fs::create_dir_all(target_dir).expect("Couldn't create directory");

    let sdk_filename = format!("vulkansdk-macos-minimal-{MACOS_SDK_VERSION}.tar.xz");
    let download_url = format!(
        "https://github.com/fornwall/libvulkan-minimal/releases/download/0.0.12/{sdk_filename}"
    );
    let download_path = target_dir.as_ref().join(&sdk_filename);

    let curl_status = Command::new("curl")
        .args(["--fail", "--location", "--silent", &download_url, "-o"])
        .arg(&download_path)
        .status()
        .expect("Couldn't launch curl");

    assert!(
        curl_status.success(),
        "failed to download prebuilt libraries"
    );

    let untar_status = Command::new("tar")
        .arg("xf")
        .arg(&download_path)
        .arg("--strip-components")
        .arg("1")
        .arg("-C")
        .arg(target_dir.as_ref())
        .status()
        .expect("Couldn't launch unzip");

    assert!(untar_status.success(), "failed to run unzip");
}

fn main() {
    let mut cc = cc::Build::new();

    let out_dir = std::env::var("OUT_DIR").unwrap();
    // bindgen - see https://rust-lang.github.io/rust-bindgen/tutorial-3.html
    let bindings = bindgen::Builder::default()
        .clang_arg("-DVTK_RUST_BINDGEN=1")
        // The input header we would like to generate bindings for.
        .header("native/vtk_cffi.h")
        // Tell cargo to invalidate the built crate whenever any of the included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");
    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(&out_dir);
    bindings
        .write_to_file(out_path.join("cffi_bindings.rs"))
        .expect("Couldn't write bindings!");

    // cbindgen - see https://michael-f-bryan.github.io/rust-ffi-guide/cbindgen.html
    let generated_headers_dir = format!("{}/headers", out_dir);
    cc.include(&generated_headers_dir);
    let output_file = format!("{}/rustffi.h", generated_headers_dir);
    let mut cbindgen_config = cbindgen::Config::default();
    cbindgen_config
        .defines
        .insert("VTK_CUSTOM_VULKAN_TYPES".to_string(), "1".to_string());
    cbindgen_config.language = cbindgen::Language::C;
    cbindgen_config.macro_expansion.bitflags = true;
    let crate_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    cbindgen::generate_with_config(&crate_dir, cbindgen_config)
        .unwrap()
        .write_to_file(&output_file);

    #[cfg(target_os = "macos")]
    {
        let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();
        if target_os != "macos" && target_os != "ios" {
            panic!("CARGO_CFG_TARGET_OS must be either 'macos' or 'ios' target");
        }
        //let target_arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();

        let target_dir =
            Path::new(&out_dir).join(format!("vulkan-sdk-{}", MACOS_SDK_VERSION));
        download_prebuilt_molten(&target_dir);
        let mut pb: PathBuf = target_dir.into();
        pb.push("macOS/");
        let mut include_dir = pb.clone();
        include_dir.push("include");
        let mut share_dir = pb.clone();
        share_dir.push("share");
        pb.push("lib/");

        println!("cargo:rustc-link-search=native={}", pb.display());

        // Setup so that vulkan validation layers can be loaded easily.
        #[cfg(feature = "validation")]
        {
            // Link to the libvulkan.dylib loader:
            println!("cargo:rustc-link-lib=dylib=vulkan");
            // See https://vulkan.lunarg.com/doc/sdk/1.3.268.0/windows/layer_configuration.html
            let vtk_icd_filenames = format!("{}/vulkan/icd.d/MoltenVK_icd.json", share_dir.display());
            cc.define("VTK_ICD_FILENAMES", Some(&vtk_icd_filenames[..]));
            let vtk_layer_path = format!("{}/vulkan/explicit_layer.d", share_dir.display());
            cc.define("VTK_LAYER_PATH", Some(&vtk_layer_path[..]));
        }
        #[cfg(not(feature = "validation"))]
        {
            // Link to MoltenVK statically.
            println!("cargo:rustc-link-lib=static=MoltenVK");
            cc.define("VTK_NO_VULKAN_LOADING", "1");
        }

        cc.include(&include_dir);

        println!("cargo:rerun-if-changed=native/platforms/mac/vtk_mac.m");
        cc.file("native/platforms/mac/vtk_mac.m");
        println!("cargo:rerun-if-changed=native/platforms/mac/VtkViewController.m");
        cc.file("native/platforms/mac/VtkViewController.m");

        cc.flag("-xobjective-c");
    }

    println!("cargo:rustc-link-lib=framework=Metal");
    println!("cargo:rustc-link-lib=framework=AppKit");
    println!("cargo:rustc-link-lib=framework=QuartzCore");
    println!("cargo:rustc-link-lib=framework=IOKit");
    println!("cargo:rustc-link-lib=framework=IOSurface");
    println!("cargo:rustc-link-lib=dylib=c++");

    #[cfg(feature = "validation")]
    cc.define("VTK_ENABLE_VULKAN_VALIDATION_LAYERS", "1");

    println!("cargo:rerun-if-changed=native/vulkan_wrapper.h");
    println!("cargo:rerun-if-changed=native/vulkan_wrapper.c");
    cc.file("native/vulkan_wrapper.c");

    cc.include("native/");
    println!("cargo:rerun-if-changed=native/vtk_cffi.h");
    println!("cargo:rerun-if-changed=native/vtk_cffi.c");
    cc.file("native/vtk_cffi.c");
    println!("cargo:rerun-if-changed=native/vtk_vulkan_setup.c");
    cc.file("native/vtk_vulkan_setup.c");

    // TODO: Make sanitize a feature or depend on build profile?
    //       Probably feature, to avoid flexibility
    //cc.flag("-Wall");
    //cc.flag("-Wextra");
    //cc.flag("-Werror");
    cc.flag("-g");
    //cc.flag("-fsanitize=address");
    //cc.flag("-fsanitize=undefined");

    cc.compile("foo");
}
