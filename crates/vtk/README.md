# vtk - Vulkan Toolkit
A toy library to use Vulkan in Rust.

# Using sanitizer
To debug memory issues in the C code - run (see [The Rust Unstable Book: sanitizer](https://doc.rust-lang.org/beta/unstable-book/compiler-flags/sanitizer.html)):

```sh
RUSTFLAGS=-Zsanitizer=address cargo +nightly run -Zbuild-std --target aarch64-apple-darwin
```
