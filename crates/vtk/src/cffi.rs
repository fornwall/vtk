#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

pub trait VtkApplication {
    fn setup_window(&mut self, device: &mut VtkDevice, window: &mut VtkWindow);
    fn render_frame(&mut self, device: &mut VtkDevice, window: &mut VtkWindow);
}

pub struct VtkContext {
    native_handle: *mut VtkContextNative,
}

impl VtkContext {
    pub fn new(mut application: Box<dyn VtkApplication>) -> Self {
        let application_pointer = Box::into_raw(Box::new(application));
        let native_handle =
            unsafe { vtk_context_init(application_pointer as *mut ::std::os::raw::c_void) };
        //assert!(okklklk);
        Self { native_handle }
    }

    pub fn create_device(&mut self) -> VtkDevice {
        let native_handle = unsafe { vtk_device_init(self.native_handle) };
        VtkDevice { native_handle }
    }

    /// Request a window to be opened.
    ///
    /// The request will be processed when `run()` is called.
    pub fn request_window(&mut self, device: &mut VtkDevice) {
        unsafe { vtk_window_init(device.native_handle) };
    }

    pub fn run(&mut self) {
        unsafe { vtk_context_run(self.native_handle) };
    }
}

pub struct VtkDevice {
    pub(crate) native_handle: *mut VtkDeviceNative,
}

pub struct VtkWindow {
    pub(crate) native_handle: *mut VtkWindowNative,
}

include!(concat!(env!("OUT_DIR"), "/cffi_bindings.rs"));
