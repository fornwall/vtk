enum GamepadButton {
    ActionDown,
    ActionRight,
    ActionLeft,
    ActionUp,
    FrontLeftUpper,
    FrontRightUpper,
    FrontLeftLower,
    FrontRightLower,
    LeftCenterCluster,
    RightCenterCluster,
    LeftStick,
    RightStick,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
    Mode,
};

enum GamepadEventType {
    ButtonPressed,
    AxisChanged,
};

union GamepadEventData {
    GamepadButton button;
    float axis_value;
};

struct GamepadEvent {
    enum TypeTag type;
    union Value value;
};

struct Gamepads;

void gamepad_callback(void (*ptr)())

struct Gamepads* gamepads_create(void* context);
