MOLTENVK_PATH = ${HOME}/bin/vulkan-sdk/MoltenVK

run:
	env | grep VULK
	RUST_BACKTRACE=1 cargo run

android:
	cargo ndk -t arm64-v8a -o ./android/app/src/main/jniLibs build --release

out:
	mkdir out

# ~/bin/vulkan-sdk/MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64/libMoltenVK.a
mac: out
	cc \
		platforms/mac/main_osx.m \
		platforms/mac/DemoViewController.m \
		-o out/main_osx \
		-framework Cocoa \
		-framework CoreVideo \
		-framework QuartzCore \
		-I$(MOLTENVK_PATH)/include \
		-L${HOME}/bin/vulkan-sdk/MoltenVK/MoltenVK.xcframework/macos-arm64_x86_64 \
		-lMoltenVK
	./out/main_osx

.PHONY: android run mac
