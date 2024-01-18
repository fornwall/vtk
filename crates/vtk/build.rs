use std::path::{Path, PathBuf};
use std::process::Command;

pub(crate) fn download_minimal_vulkan_sdk(out_dir: &str, directory_name: &str) -> PathBuf {
    const MINIMAL_VULKAN_SDK_VERSION: &str = "0.0.19";
    let sdk_dir = Path::new(&out_dir).join(format!("vulkan-sdk-{}", MINIMAL_VULKAN_SDK_VERSION));
    if sdk_dir.exists() {
        return sdk_dir;
    }

    let sdk_filename = format!("{directory_name}.tar.xz");
    let download_path = Path::new(&out_dir).join(&sdk_filename);

    if !download_path.exists() {
        let download_url = format!("https://github.com/fornwall/libvulkan-minimal/releases/download/{MINIMAL_VULKAN_SDK_VERSION}/{sdk_filename}");
        // TODO: Checksum check
        let curl_status = Command::new("curl")
            .args(["--fail", "--location", "--silent", &download_url, "-o"])
            .arg(&download_path)
            .status()
            .expect("Couldn't launch curl");
        assert!(
            curl_status.success(),
            "failed to download prebuilt libraries: {download_url}"
            );
    }

    let sdk_tmp_dir = Path::new(&out_dir).join(format!("vulkan-sdk-{}.tmp", MINIMAL_VULKAN_SDK_VERSION));
    std::fs::create_dir_all(&sdk_tmp_dir).expect("Couldn't create directory");
    let untar_status = Command::new("tar")
        .arg("xf")
        .arg(&download_path)
        .arg("--strip-components")
        .arg("1")
        .arg("-C")
        .arg(&sdk_tmp_dir)
        .status()
        .expect("Couldn't launch unzip");

    assert!(untar_status.success(), "failed to run unzip");
    std::fs::rename(&sdk_tmp_dir, &sdk_dir).unwrap();
    sdk_dir
}

fn main() {
    let build_target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();

    let mut cc = cc::Build::new();

    let build_c_file = |builder: &mut cc::Build, file_path| {
        println!("cargo:rerun-if-changed={file_path}");
        builder.file(file_path);
    };

    let out_dir = std::env::var("OUT_DIR").unwrap();

    let generated_headers_dir = format!("{}/headers", out_dir);

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

    if build_target_os == "android"
    {
        // Several Android NDK r26 headers are C++ only: https://github.com/android/ndk/issues/1920
        cc.cpp(true);
        cc.cpp_link_stdlib(None);

        build_c_file(&mut cc, "native/vtk_android.c");
    } else if build_target_os == "macos" || build_target_os == "ios" {
        // let target_arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();
        let target_dir = download_minimal_vulkan_sdk(&out_dir, "vulkansdk-macos-minimal");
        let vulkan_sdk_include_dir = format!("{}/include", target_dir.display());
        let vulkan_sdk_lib_dir = format!("{}/lib", target_dir.display());

        #[cfg(feature = "validation")]
        {
            // Setup so that vulkan validation layers can be loaded easily.
            // Link to the libvulkan.dylib loader:
            println!("cargo:rustc-link-lib=dylib=vulkan");
            // See https://vulkan.lunarg.com/doc/sdk/1.3.268.0/windows/layer_configuration.html
            let vulkan_sdk_share_dir = format!("{}/share", target_dir.display());
            let vtk_icd_filenames =
                format!("{}/vulkan/icd.d/MoltenVK_icd.json", vulkan_sdk_share_dir);
            let vtk_layer_path = format!("{}/vulkan/explicit_layer.d", vulkan_sdk_share_dir);
            cc.define("VTK_ICD_FILENAMES", Some(&vtk_icd_filenames[..]));
            cc.define("VTK_LAYER_PATH", Some(&vtk_layer_path[..]));
        }
        #[cfg(not(feature = "validation"))]
        {
            // Link to MoltenVK statically if not using validation layers.
            println!("cargo:rustc-link-lib=static=MoltenVK");
            cc.define("VTK_NO_VULKAN_LOADING", "1");
        }

        cc.include(&vulkan_sdk_include_dir);

        println!("cargo:rustc-link-lib=dylib=c++");
        println!("cargo:rustc-link-lib=framework=AppKit");
        println!("cargo:rustc-link-lib=framework=IOKit");
        println!("cargo:rustc-link-lib=framework=IOSurface");
        println!("cargo:rustc-link-lib=framework=Metal");
        println!("cargo:rustc-link-lib=framework=QuartzCore");
        println!("cargo:rustc-link-search=native={}", vulkan_sdk_lib_dir);

        cc.flag("-xobjective-c");

        build_c_file(&mut cc, "native/vtk_mac.c");
        build_c_file(&mut cc, "native/platforms/mac/VtkViewController.m");
    } else if build_target_os == "linux" {
        let target_dir = download_minimal_vulkan_sdk(&out_dir, "vulkansdk-linux-minimal-x86_64");
        let mut pb = PathBuf::from(
            std::env::var("CARGO_MANIFEST_DIR").expect("unable to find env:CARGO_MANIFEST_DIR"),
        );
        pb.push(target_dir);

        let mut include_dir = pb.clone();
        include_dir.push("include");
        cc.include(&include_dir);

        let mut lib_dir = pb.clone();
        lib_dir.push("lib");
        println!("cargo:rustc-link-search=native={}", lib_dir.display());

        // Link to the libvulkan.so loader:
        println!("cargo:rustc-link-lib=dylib=vulkan");
        // Link to wayland-client
        println!("cargo:rustc-link-lib=dylib=wayland-client");
        // Link to xkbcommon:
        println!("cargo:rustc-link-lib=dylib=xkbcommon");

        #[cfg(feature = "validation")]
        {
            // Setup so that vulkan validation layers can be loaded easily.
            // See https://vulkan.lunarg.com/doc/sdk/1.3.268.0/windows/layer_configuration.html
            let vtk_layer_path = format!("{}/etc/vulkan/explicit_layer.d", pb.display());
            cc.define("VTK_LAYER_PATH", Some(&vtk_layer_path[..]));
        }

        build_c_file(&mut cc, "native/vtk_wayland.c");
        build_c_file(&mut cc, "native/vtk_wayland_keyboard.c");

        // See https://wayland-book.com/xdg-shell-basics/example-code.html
        let generated_wayland_c = format!("{out_dir}/xdg-shell-client-protocol.c");
        assert!(Command::new("sh")
            .args([
                "-c",
                &format!("wayland-scanner private-code < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > {generated_wayland_c}")
            ])
            .status()
            .unwrap()
            .success());
        assert!(Command::new("sh")
            .args([
                "-c",
                &format!("wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > {generated_headers_dir}/xdg-shell-client-protocol.h")
            ])
            .status()
            .unwrap()
            .success());
        cc.file(generated_wayland_c);
    }

    #[cfg(feature = "validation")]
    cc.define("VTK_VULKAN_VALIDATION", "1");

    cc.include("native/");
    cc.include(&generated_headers_dir);

    println!("cargo:rerun-if-changed=native/vulkan_wrapper.h");
    println!("cargo:rerun-if-changed=native/vtk_cffi.h");

    build_c_file(&mut cc, "native/vulkan_wrapper.c");
    build_c_file(&mut cc, "native/vtk_cffi.c");
    build_c_file(&mut cc, "native/vtk_vulkan_setup.c");

    // TODO: Make sanitize a feature or depend on build profile?
    //       Probably feature, to avoid flexibility
    //cc.flag("-Wall");
    //cc.flag("-Wextra");
    //cc.flag("-Werror");
    cc.flag("-g");
    //cc.flag("-fsanitize=address");
    //cc.flag("-fsanitize=undefined");

    cc.compile("vtk");
}
