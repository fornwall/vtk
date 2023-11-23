use crate::{cffi, VtkDevice, VtkDeviceNative, VtkWindow, VtkWindowNative};
/// The `rustffi` - API that rust expose to C.
use bitflags::bitflags;
use cffi::VtkApplication;
use std::ffi::c_char;

bitflags! {
    /// Represents a set of flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
    #[repr(C)]
    pub struct Key: u32 {
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

#[no_mangle]
pub extern "C" fn vtk_application_setup_window(
    application: *mut ::std::os::raw::c_void,
    vtk_device: *mut ::std::os::raw::c_void,
    vtk_window: *mut ::std::os::raw::c_void,
) {
    let mut application = unsafe { Box::from_raw(application as *mut Box<dyn VtkApplication>) };
    let vtk_device: &mut VtkDeviceNative = unsafe { &mut *(vtk_device as *mut VtkDeviceNative) };
    let vtk_window: &mut VtkWindowNative = unsafe { &mut *(vtk_window as *mut VtkWindowNative) };
    let mut vtk_device = VtkDevice {
        native_handle: vtk_device,
    };
    let mut vtk_window = VtkWindow {
        native_handle: vtk_window,
    };
    application.setup_window(&mut vtk_device, &mut vtk_window);
    Box::leak(application);
}

#[no_mangle]
pub extern "C" fn vtk_application_render_frame(
    application: *mut ::std::os::raw::c_void,
    vtk_device: *mut ::std::os::raw::c_void,
    vtk_window: *mut ::std::os::raw::c_void,
) {
    //let application : &mut Box<dyn VtkApplication> = unsafe { &mut *(application as *mut Box<dyn VtkApplication>) };
    let mut application = unsafe { Box::from_raw(application as *mut Box<dyn VtkApplication>) };
    let vtk_device: &mut VtkDeviceNative = unsafe { &mut *(vtk_device as *mut VtkDeviceNative) };
    let vtk_window: &mut VtkWindowNative = unsafe { &mut *(vtk_window as *mut VtkWindowNative) };
    let mut vtk_device = VtkDevice {
        native_handle: vtk_device,
    };
    let mut vtk_window = VtkWindow {
        native_handle: vtk_window,
    };
    application.render_frame(&mut vtk_device, &mut vtk_window);
    Box::leak(application);
}
