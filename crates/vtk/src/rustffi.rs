use crate::{cffi, VtkDevice, VtkDeviceNative, VtkWindow, VtkWindowNative};
/// The `rustffi` - API that rust expose to C.
use bitflags::bitflags;
use std::ffi::c_char;

bitflags! {
    /// Represents a set of flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
    #[repr(C)]
    pub struct Key: u64 {
        const ArrowUp = 1;
        const ArrowRight = 1 << 1;
        const ArrowDown = 1 << 2;
        const ArrowLeft = 1 << 3;
        const Shift = 1 << 4;
        const CapsLock = 1 << 5;
        const Command = 1 << 6;
        const Control = 1 << 7;
        const A = 1 << 10;
        const D = 1 << 11;
        const S = 1 << 12;
        const W = 1 << 13;

        // The combination of `A`, `B`, and `C`.
        //const ABC = Self::A.bits() | Self::B.bits() | Self::C.bits();
    }
}

// pub static VERSION: &str = concat!(env!("CARGO_VERSION"), "\0");

#[no_mangle]
pub extern "C" fn get_triangle_fragment_shader() -> *const c_char {
    //let bytes = include_bytes!("../out/shaders/triangle.vert.spv");
    //bytes.as_ptr() as *const c_char
    "hello, world".as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn add_held_keys(held_keys: Key) {
    for key in held_keys.iter() {
        println!("KEY HELD: {:?}", key);
        match key {
            Key::ArrowUp => {
                println!("ArrowUp from rust");
            }
            Key::ArrowRight => {
                println!("ArrowRight from rust");
            }
            _ => {}
        }
    }
}