CARGO_BUILD_COMMAND = cargo build
GLSLC = glslc -I shaders --target-env=vulkan1.3
MOLTENVK_PATH = ${HOME}/bin/vulkan-sdk/MoltenVK
CC = cc -Wall -Wextra

ifeq ($(SANITIZE),1)
	CC += -fsanitize=address -fsanitize=undefined
endif

ifeq ($(RELEASE),1)
	CC += -O2
	RUST_LIB_DIR = target/release
	CARGO_BUILD_COMMAND += --release
else
	CC += -g
	RUST_LIB_DIR = target/debug
endif


install-dependencies:
	brew install shaderc # For glslc
	cargo install bindgen-cli
	cargo install cbindgen

run:
	env | grep VULK
	RUST_BACKTRACE=1 cargo run

android:
	cargo ndk -t arm64-v8a -o ./platforms/android/app/src/main/jniLibs build --release
	cd platforms/android && gradle assembleDebug

out/shaders:
	mkdir -p out/shaders

shaders: out/shaders out/shaders/triangle.frag.spv out/shaders/triangle.vert.spv

out/shaders/%.frag.spv : shaders/%.frag
	$(GLSLC) $< -o $@

out/shaders/%.vert.spv : shaders/%.vert
	$(GLSLC) $< -o $@

rustlib:
	$(CARGO_BUILD_COMMAND)
	rm $(RUST_LIB_DIR)/libvulkan_example.dylib

c-ffi:
	bindgen vulkan/ffi.h > src/ffi.rs

rust-ffi:
	@mkdir -p out/headers
	cbindgen --config cbindgen.toml --output out/headers/rust-ffi.h

# ~/bin/vulkan-sdk/MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64/libMoltenVK.a
mac: shaders rustlib rust-ffi
	$(CC) \
		platforms/mac/main_osx.m \
		platforms/mac/CustomViewController.m \
		vulkan/vulkan_main.m \
		-o out/main_osx \
		-framework Cocoa \
		-framework IOKit \
		-framework IOSurface \
		-framework Metal \
		-framework QuartzCore \
		-I$(MOLTENVK_PATH)/include \
		-Ivulkan/ \
		-Iout/headers \
		-L${HOME}/bin/vulkan-sdk/MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64 \
		-L$(RUST_LIB_DIR) \
		-lMoltenVK \
		-lvulkan_example \
		-lc++
	./out/main_osx

clean:
	rm -Rf out

.PHONY: android rustlib run mac shaders install-dependencies clean
