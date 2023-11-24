mod cffi;
mod rustffi;

pub use cffi::*;
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

#[derive(Copy, Clone)]
enum KeyCode {
    A,
    B,
    C,
    D,
}

enum KeyModifiers {
    Ctrl,
    Alt,
    Cmd,
}

struct KeyPressed {

}

enum MainThreadEvent {

}

struct TickInput {}
