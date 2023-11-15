#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/cffi_bindings.rs"));

pub struct VtkApplication {
    native_handle: *mut VtkApplicationNative,
}

impl VtkApplication {
    pub fn new() -> Self {
        let native_handle = unsafe { vtk_application_init() };
        //assert!(okklklk);
        Self {
            native_handle
        }
    }

    pub fn run(&mut self) {
        unsafe { vtk_application_run(self.native_handle) };
    }
}

pub struct VtkDevice {
    native_handle: *mut VtkDeviceNative,
}

impl VtkDevice {
    pub fn new(application: &mut VtkApplication) -> Self {
        let native_handle = unsafe { vtk_device_init(application.native_handle) };
        //assert!(ok);
        Self {
            native_handle
        }
    }
}

pub struct VtkWindow {
    native_handle: *mut VtkWindowNative
}

impl VtkWindow {
    pub fn new<F: FnOnce(&mut VtkDevice) -> ()>(device: &mut VtkDevice, setup_function: F) -> Self {
        let native_handle = unsafe { vtk_window_init(device.native_handle) };
        //assert!(ok);
        Self {
            native_handle
        }
    }
    pub fn debug_print(&self) {
        unsafe {
            println!("device in debug print: {:p}", (*self.native_handle).vtk_device);
            println!("width,height debug print: {},{}", (*self.native_handle).width, (*self.native_handle).height);
        }
    }
}
