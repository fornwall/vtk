#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

pub struct VtkContext {
    native_handle: *mut VtkContextNative,
}

impl VtkContext {
    pub fn new() -> Self {
        let native_handle = unsafe { vtk_context_init() };
        Self { native_handle }
    }

    pub fn create_device(&mut self) -> VtkDevice {
        let native_handle = unsafe { vtk_device_init(self.native_handle) };
        VtkDevice { native_handle }
    }

    /// Request a window to be opened.
    ///
    /// The request will be processed when `run()` is called.
    pub fn create_window(&mut self, device: &mut VtkDevice) -> VtkWindow {
        let native_handle = unsafe { vtk_window_init(device.native_handle) };
        VtkWindow {
            native_handle
        }
    }

    pub fn run(&mut self) {
        unsafe { vtk_context_run(self.native_handle) };
    }
}

pub struct VtkDevice {
    pub(crate) native_handle: *mut VtkDeviceNative,
}

unsafe impl Send for VtkDevice {}

impl VtkDevice {
    fn create_shader(&self, spirv_bytes: &[u8]) -> VtkShaderModule {
        let vulkan_handle = unsafe {
            vtk_device_create_shader(self.native_handle, spirv_bytes.as_ptr(), spirv_bytes.len())
        };
        VtkShaderModule {
            vulkan_handle
        }
    }
}

pub struct VtkWindow {
    pub(crate) native_handle: *mut VtkWindowNative,
}

unsafe impl Send for VtkWindow {}

impl VtkWindow {
    pub fn render(&mut self) {
        unsafe {
            vtk_render_frame(self.native_handle);
        }
    }
}

pub struct VtkShaderModule {
    vulkan_handle: u64,
}

pub struct VtkBufferAllocator {
    pointer: *mut std::ffi::c_void,
    size: u64,
    buffer: VkBuffer,
    memory: VkDeviceMemory,
}

include!(concat!(env!("OUT_DIR"), "/cffi_bindings.rs"));
