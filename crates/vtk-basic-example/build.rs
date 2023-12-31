fn main() {
    let crate_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    let out_dir = std::env::var("OUT_DIR").unwrap();

    let mut out_path = std::path::PathBuf::from(&out_dir);
    out_path.push("shaders");

    std::fs::create_dir_all(&out_path).unwrap();

    for extension in ["vert", "frag"] {
        let in_file = format!("{}/shaders/triangle.{}", crate_dir, extension);
        let out_file = format!("{}/triangle.{}.spv", out_path.display(), extension);
        println!("cargo:rerun-if-changed={}", in_file);

        let status = std::process::Command::new("glslc")
            .args([
                "-I",
                "shaders",
                "--target-env=vulkan1.3",
                "-o",
                &out_file,
                &in_file,
            ])
            .status()
            .expect("Couldn't launch glslc");

        assert!(status.success(), "Failed running glslc to compile shaders");
    }
}
