mod cffi;
mod rustffi;

pub use cffi::*;
use rustffi::Key;

impl VtkDevice {
    pub fn new() -> Self {
        let mut result = Self {
            initialized: false,
            vk_instance: std::ptr::null_mut(),
            vk_physical_device: std::ptr::null_mut(),
            vk_device: std::ptr::null_mut(),
            queue_family_index: 0,
            vk_queue: std::ptr::null_mut(),
        };
        let ok = unsafe { vtk_device_init(&mut result as *mut Self) };
        assert!(ok);
        result
    }
}

struct KeyInput {
    bits: Key,
}

impl KeyInput {
    fn is_held(self, key: Key) -> bool {
        self.bits.contains(key)
    }

    fn all_pressed(self) -> bitflags::iter::Iter<Key> {
        self.bits.iter()
    }
}

struct TickInput {}

struct Application {}

impl Application {
    fn on_tick(&self) {
        // TODO
    }

    fn render_frame(&self) {
        // TODO
    }
}
