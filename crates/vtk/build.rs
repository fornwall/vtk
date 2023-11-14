use std::path::{Path, PathBuf};

// https://github.com/EmbarkStudios/ash-molten/tree/main/build/xcframework
#[cfg(target_os = "macos")]
mod xcframework {
    use anyhow::Error;
    use std::path::{Path, PathBuf};
    use std::{fs::File, io::BufReader, process::Command, string::String};

    #[derive(Debug, Clone, Copy, Hash, PartialEq, Eq, serde::Deserialize)]
    #[serde(into = "&'static str")]
    #[serde(from = "String")]
    pub enum Arch {
        X86,
        Amd64,
        Arm64,
        Arm64e,
        Unknown,
    }

    #[derive(Debug, Clone, Copy, Hash, PartialEq, Eq, serde::Deserialize)]
    #[serde(into = "&'static str")]
    #[serde(from = "String")]
    pub enum Platform {
        MacOs,
        Ios,
        TvOs,
        WatchOs,
        Unknown,
    }

    #[derive(Debug, Clone, Copy, Hash, PartialEq, Eq, serde::Deserialize)]
    #[serde(into = "&'static str")]
    #[serde(from = "String")]
    pub enum Variant {
        Default,
        Simulator,
    }

    impl<T: AsRef<str>> From<T> for Arch {
        fn from(arch: T) -> Self {
            match arch.as_ref() {
                "x86_64" => Arch::Amd64,
                "x86" => Arch::X86,
                "arm64" | "aarch64" => Arch::Arm64,
                "arm64e" => Arch::Arm64e,
                _ => Arch::Unknown,
            }
        }
    }

    impl<'a> From<Arch> for &'a str {
        fn from(arch: Arch) -> Self {
            match arch {
                Arch::Amd64 => "x86_64",
                Arch::X86 => "x86",
                Arch::Arm64 => "arm64",
                Arch::Arm64e => "arm64e",
                Arch::Unknown => "",
            }
        }
    }

    impl<T: AsRef<str>> From<T> for Platform {
        fn from(platform: T) -> Self {
            match platform.as_ref() {
                "tvos" => Platform::TvOs,
                "macos" => Platform::MacOs,
                "ios" => Platform::Ios,
                "watchos" => Platform::WatchOs,
                _ => Platform::Unknown,
            }
        }
    }

    impl<'a> From<Platform> for &'a str {
        fn from(platform: Platform) -> Self {
            match platform {
                Platform::TvOs => "tvos",
                Platform::MacOs => "macos",
                Platform::Ios => "ios",
                Platform::WatchOs => "watchos",
                Platform::Unknown => "",
            }
        }
    }

    impl<T: AsRef<str>> From<T> for Variant {
        fn from(variant: T) -> Self {
            match variant.as_ref() {
                "simulator" => Variant::Simulator,
                _ => Variant::Default,
            }
        }
    }

    impl<'a> From<Variant> for &'a str {
        fn from(variant: Variant) -> Self {
            match variant {
                Variant::Simulator => "simulator",
                Variant::Default => "",
            }
        }
    }

    #[derive(Debug, Clone, Copy, Hash, PartialEq, Eq)]
    pub struct Identifier {
        pub arch: Arch,
        pub platform: Platform,
        pub variant: Variant,
    }

    impl Identifier {
        pub fn new(arch: Arch, platform: Platform, variant: Variant) -> Self {
            Self {
                arch,
                platform,
                variant,
            }
        }
    }

    #[allow(non_snake_case)]
    #[derive(Debug, serde::Deserialize)]
    pub struct UniversalLibrary {
        LibraryPath: String,
        SupportedArchitectures: Vec<Arch>,
        SupportedPlatformVariant: Option<Variant>,
        SupportedPlatform: Platform,
        LibraryIdentifier: String,
    }

    #[allow(non_snake_case)]
    #[derive(Debug)]
    pub struct NativeLibrary {
        LibraryPath: String,
        SupportedArchitectures: Arch,
        SupportedPlatformVariant: Option<Variant>,
        SupportedPlatform: Platform,
        LibraryIdentifier: String,
    }

    impl UniversalLibrary {
        pub fn universal_to_native<P: AsRef<Path>>(
            self,
            xcframework_dir: P,
        ) -> Result<Vec<NativeLibrary>, Error> {
            let lib_id = &self.LibraryIdentifier;
            let platform = &self.SupportedPlatform;
            let variant = self.SupportedPlatformVariant.as_ref();
            let lib_path = &self.LibraryPath;

            if self.SupportedArchitectures.len() == 1 {
                let arch = &self.SupportedArchitectures[0];
                Ok(vec![NativeLibrary {
                    LibraryPath: lib_path.into(),
                    SupportedArchitectures: *arch,
                    SupportedPlatformVariant: variant.cloned(),
                    SupportedPlatform: *platform,
                    LibraryIdentifier: lib_id.into(),
                }])
            } else {
                let mut native_libs = Vec::new();
                let xcframework_dir = xcframework_dir.as_ref().to_path_buf();
                let full_path = xcframework_dir.join(lib_id).join(lib_path);

                for arch in &self.SupportedArchitectures {
                    let platform_str: &str = (*platform).into();
                    let arch_str: &str = (*arch).into();
                    let mut new_identifier = format!("{}-{}", platform_str, arch_str);

                    if let Some(variant) = variant {
                        let variant_str: &str = (*variant).into();
                        new_identifier.push_str(format!("-{}", variant_str).as_str());
                    }

                    let mut out_path = xcframework_dir.join(new_identifier.clone());
                    std::fs::create_dir_all(&out_path)?;
                    out_path.push(lib_path);

                    assert!(Command::new("lipo")
                        .arg(&full_path)
                        .arg("-thin")
                        .arg(arch_str)
                        .arg("-output")
                        .arg(out_path)
                        .status()
                        .expect("Failed to spawn lipo")
                        .success());

                    native_libs.push(NativeLibrary {
                        LibraryPath: lib_path.into(),
                        SupportedArchitectures: *arch,
                        SupportedPlatformVariant: variant.copied(),
                        SupportedPlatform: *platform,
                        LibraryIdentifier: new_identifier,
                    });
                }

                Ok(native_libs)
            }
        }
    }

    impl NativeLibrary {
        pub fn path(&self) -> PathBuf {
            Path::new(&format!("{}/{}", self.LibraryIdentifier, self.LibraryPath)).to_path_buf()
        }

        pub fn identifier(&self) -> Identifier {
            Identifier::new(
                self.SupportedArchitectures,
                self.SupportedPlatform,
                self.SupportedPlatformVariant.unwrap_or(Variant::Default),
            )
        }
    }

    #[allow(non_snake_case)]
    #[derive(Debug, serde::Deserialize)]
    pub struct XcFramework {
        pub AvailableLibraries: Vec<UniversalLibrary>,
        pub CFBundlePackageType: String,
        pub XCFrameworkFormatVersion: String,
    }

    impl XcFramework {
        pub fn parse<P: AsRef<Path>>(path: P) -> Result<XcFramework, Error> {
            let mut reader = BufReader::new(File::open(path.as_ref().join("Info.plist"))?);
            Ok(plist::from_reader(&mut reader)?)
        }
    }
}

pub const MOLTEN_VK_VERSION: &str = "1.2.6";

pub(crate) fn download_prebuilt_molten<P: AsRef<Path>>(target_dir: &P) {
    use std::process::Command;

    std::fs::create_dir_all(target_dir).expect("Couldn't create directory");

    let download_url = format!(
        "https://github.com/KhronosGroup/MoltenVK/releases/download/v{}/MoltenVK-macos.tar",
        MOLTEN_VK_VERSION
    );
    let download_path = target_dir.as_ref().join("MoltenVK.xcframework.zip");

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
        .arg("-C")
        .arg(target_dir.as_ref())
        .status()
        .expect("Couldn't launch unzip");

    assert!(untar_status.success(), "failed to run unzip");
}

fn main() {
    let mut cc = cc::Build::new();

    #[cfg(target_os = "macos")]
    {
        let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();
        if target_os != "macos" && target_os != "ios" {
            panic!("CARGO_CFG_TARGET_OS must be either 'macos' or 'ios' target");
        }
        let target_arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();

        let target_dir = Path::new(&std::env::var("OUT_DIR").unwrap())
            .join(format!("Prebuilt-MoltenVK-{}", MOLTEN_VK_VERSION));
        download_prebuilt_molten(&target_dir);
        let mut pb = PathBuf::from(
            std::env::var("CARGO_MANIFEST_DIR").expect("unable to find env:CARGO_MANIFEST_DIR"),
        );
        pb.push(target_dir);
        pb.push("MoltenVK/MoltenVK/");
        let mut include_dir = pb.clone();
        include_dir.push("include");
        cc.include(&include_dir);
        pb.push("MoltenVK.xcframework");
        let mut project_dir = pb;
        let xcframework = xcframework::XcFramework::parse(&project_dir)
            .unwrap_or_else(|_| panic!("Failed to parse XCFramework from {project_dir:?}"));
        let native_libs = xcframework
            .AvailableLibraries
            .into_iter()
            .flat_map(|lib| {
                lib.universal_to_native(project_dir.clone())
                    .expect("Failed to get native library")
            })
            .map(|lib| (lib.identifier(), lib.path()))
            .collect::<std::collections::HashMap<
                xcframework::Identifier,
                PathBuf,
                std::collections::hash_map::RandomState,
            >>();
        let id = xcframework::Identifier::new(
            target_arch.into(),
            target_os.into(),
            xcframework::Variant::Default,
        );
        let lib_path = native_libs.get(&id).expect("Library was not found");
        let lib_dir = lib_path.parent().unwrap();
        project_dir.push(lib_dir);
        println!("cargo:rustc-link-search=native={}", project_dir.display());
    }

    println!("cargo:rustc-link-lib=framework=Metal");
    println!("cargo:rustc-link-lib=framework=AppKit");
    println!("cargo:rustc-link-lib=framework=QuartzCore");
    println!("cargo:rustc-link-lib=framework=IOKit");
    println!("cargo:rustc-link-lib=framework=IOSurface");
    println!("cargo:rustc-link-lib=dylib=c++");
    println!("cargo:rustc-link-lib=static=MoltenVK");

    cc.file("native/vtk_cffi.c");
    cc.compile("foo");
}
