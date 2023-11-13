mod cffi;
mod rustffi;
mod shaders;

use rustffi::Key;

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
