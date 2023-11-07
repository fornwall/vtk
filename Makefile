CARGO_BUILD_COMMAND = cargo build
GLSLC = glslangValidator
MOLTENVK_PATH = ${HOME}/bin/vulkan-sdk/MoltenVK

ifeq ($(RELEASE),1)
	RUST_LIB_DIR = target/release
	CARGO_BUILD_COMMAND += --release
else
	RUST_LIB_DIR = target/debug
endif


run:
	env | grep VULK
	RUST_BACKTRACE=1 cargo run

android:
	cargo ndk -t arm64-v8a -o ./android/app/src/main/jniLibs build --release

out/shaders:
	mkdir -p out/shaders

shaders: out/shaders out/shaders/triangle.frag.spv out/shaders/triangle.vert.spv

out/shaders/%.frag.spv : shaders/%.frag
	$(GLSLC) -V $< -o $@

out/shaders/%.vert.spv : shaders/%.vert
	$(GLSLC) -V $< -o $@

rustlib:
	$(CARGO_BUILD_COMMAND)

rust-ffi:
	@mkdir -p out/headers
	cbindgen --config cbindgen.toml --output out/headers/rust-ffi.h

# ~/bin/vulkan-sdk/MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64/libMoltenVK.a
mac: shaders rustlib rust-ffi
	cc \
		platforms/mac/main_osx.m \
		platforms/mac/DemoViewController.m \
		-o out/main_osx \
		-framework Cocoa \
		-framework CoreVideo \
		-framework QuartzCore \
		-I$(MOLTENVK_PATH)/include \
		-Iout/headers \
		-L${HOME}/bin/vulkan-sdk/MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64 \
		-Ltarget/debug \
		-lMoltenVK \
		-lvulkan_example
	./out/main_osx

.PHONY: android rustlib run mac shaders
