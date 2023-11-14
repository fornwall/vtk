// Check https://stackoverflow.com/questions/2234364/how-to-detect-a-gamepad-button-press-on-osx-10-5-and-higher
// for a working cocoa program
#include <IOKit/hid/IOHIDLib.h>
#include <ForceFeedback/ForceFeedback.h>

#define MAX_GAMEPADS 8
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define RUMBLE_MAGNITUDE_MAX 10000


const uint16_t kGenericDesktopUsagePage = 0x01;
const uint16_t kGameControlsUsagePage = 0x05;
const uint16_t kButtonUsagePage = 0x09;
const uint16_t kConsumerUsagePage = 0x0c;
const uint16_t kJoystickUsageNumber = 0x04;
const uint16_t kGameUsageNumber = 0x05;
const uint16_t kMultiAxisUsageNumber = 0x08;
const uint16_t kAxisMinimumUsageNumber = 0x30;
const uint16_t kSystemMainMenuUsageNumber = 0x85;
const uint16_t kPowerUsageNumber = 0x30;
const uint16_t kSearchUsageNumber = 0x0221;
const uint16_t kHomeUsageNumber = 0x0223;
const uint16_t kBackUsageNumber = 0x0224;
const uint16_t kRecordUsageNumber = 0xb2;

struct GamepadRef {
    IOHIDDeviceRef device_ref;
    FFDeviceObjectReference ff_device_ref;
    FFEffectObjectReference ff_effect_ref;
    FFEFFECT ff_effect;
    FFCUSTOMFORCE ff_custom_force;
    LONG force_data[2];
    DWORD axes_data[2];
    LONG direction_data[2];
};

struct GamepadData {
    uint32_t just_pressed_bits;
    uint32_t held_bits;
    float axes[4];
};

struct Gamepads {
    struct GamepadRef gamepad_refs[MAX_GAMEPADS];
    struct GamepadRef gamepad_datas[MAX_GAMEPADS];
    uint8_t num_connected_gamepads;
};

float NormalizeAxis(CFIndex value, CFIndex min, CFIndex max) {
  return (2.f * (value - min) / (float) (max - min)) - 1.f;
}

float NormalizeUInt8Axis(uint8_t value, uint8_t min, uint8_t max) {
  return (2.f * (value - min) / (float) (max - min)) - 1.f;
}

float NormalizeUInt16Axis(uint16_t value, uint16_t min, uint16_t max) {
  return (2.f * (value - min) / (float) (max - min)) - 1.f;
}

float NormalizeUInt32Axis(uint32_t value, uint32_t min, uint32_t max) {
  return (2.f * (value - min) / (float) (max - min)) - 1.f;
}

FFDeviceObjectReference CreateForceFeedbackDevice(IOHIDDeviceRef device_ref) {
  io_service_t service = IOHIDDeviceGetService(device_ref);
  if (service == MACH_PORT_NULL) return NULL;
  HRESULT res = FFIsForceFeedback(service);
  if (res != FF_OK) return NULL;
  FFDeviceObjectReference ff_device_ref;
  res = FFCreateDevice(service, &ff_device_ref);
  if (res != FF_OK) return NULL;
  return ff_device_ref;
}

static FFEffectObjectReference CreateForceFeedbackEffect(
    FFDeviceObjectReference ff_device_ref,
    FFEFFECT* ff_effect,
    FFCUSTOMFORCE* ff_custom_force,
    LONG* force_data,
    DWORD* axes_data,
    LONG* direction_data
) {
  /*
  DCHECK(ff_effect);
  DCHECK(ff_custom_force);
  DCHECK(force_data);
  DCHECK(axes_data);
  DCHECK(direction_data);
  */
  FFCAPABILITIES caps;
  HRESULT res = FFDeviceGetForceFeedbackCapabilities(ff_device_ref, &caps);
  if (res != FF_OK) return NULL;
  if ((caps.supportedEffects & FFCAP_ET_CUSTOMFORCE) == 0) return NULL;
  force_data[0] = 0;
  force_data[1] = 0;
  axes_data[0] = caps.ffAxes[0];
  axes_data[1] = caps.ffAxes[1];
  direction_data[0] = 0;
  direction_data[1] = 0;
  ff_custom_force->cChannels = 2;
  ff_custom_force->cSamples = 2;
  ff_custom_force->rglForceData = force_data;
  ff_custom_force->dwSamplePeriod = 100000;  // 100 ms
  ff_effect->dwSize = sizeof(FFEFFECT);
  ff_effect->dwFlags = FFEFF_OBJECTOFFSETS | FFEFF_SPHERICAL;
  ff_effect->dwDuration = 5000000;     // 5 seconds
  ff_effect->dwSamplePeriod = 100000;  // 100 ms
  ff_effect->dwGain = 10000;
  ff_effect->dwTriggerButton = FFEB_NOTRIGGER;
  ff_effect->dwTriggerRepeatInterval = 0;
  ff_effect->cAxes = caps.numFfAxes;
  ff_effect->rgdwAxes = axes_data;
  ff_effect->rglDirection = direction_data;
  ff_effect->lpEnvelope = NULL;
  ff_effect->cbTypeSpecificParams = sizeof(FFCUSTOMFORCE);
  ff_effect->lpvTypeSpecificParams = ff_custom_force;
  ff_effect->dwStartDelay = 0;
  FFEffectObjectReference ff_effect_ref;
  res = FFDeviceCreateEffect(ff_device_ref, kFFEffectType_CustomForce_ID, ff_effect, &ff_effect_ref);
  return (res == FF_OK) ? ff_effect_ref : NULL;
}

static void gamepad_set_vibrate(
        struct GamepadRef* gamepad,
        double strong_magnitude,
        double weak_magnitude,
        uint32_t duration_ms
) {
  FFCUSTOMFORCE* ff_custom_force = gamepad->ff_effect.lpvTypeSpecificParams;
  ff_custom_force->rglForceData[0] = (LONG) (strong_magnitude * RUMBLE_MAGNITUDE_MAX);
  ff_custom_force->rglForceData[1] = (LONG) (weak_magnitude * RUMBLE_MAGNITUDE_MAX);
  gamepad->ff_effect.dwDuration = duration_ms * 1000;
  ff_custom_force->dwSamplePeriod = duration_ms * 1000;
  // Download the effect to the device and start the effect.
  // See FFEP_STARTDELAY
  HRESULT res = FFEffectSetParameters(gamepad->ff_effect_ref, &gamepad->ff_effect, FFEP_DURATION | FFEP_STARTDELAY | FFEP_TYPESPECIFICPARAMS);
  if (res == FF_OK) FFEffectStart(gamepad->ff_effect_ref, 1, FFES_SOLO);
}

void gamepad_stop_vibration(struct GamepadRef* gamepad) {
  FFEffectStop(gamepad->ff_effect_ref);
}

// Check that a parent collection of this element matches one of the usage
// numbers that we are looking for.
static bool check_collection(IOHIDElementRef element) {
  // Check that a parent collection of this element matches one of the usage
  // numbers that we are looking for.
  while ((element = IOHIDElementGetParent(element)) != NULL) {
    uint32_t usage_page = IOHIDElementGetUsagePage(element);
    uint32_t usage = IOHIDElementGetUsage(element);
    if (usage_page == kGenericDesktopUsagePage) {
      if (usage == kJoystickUsageNumber || usage == kGameUsageNumber || usage == kMultiAxisUsageNumber) {
        return true;
      }
    }
  }
  return false;
}


static void gamepad_added_callback(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef device_ref) {
  printf("Gamepad was plugged in\n");

  // See for more force feedback:
  // https://chromium.googlesource.com/chromium/src/+/HEAD/device/gamepad/gamepad_device_mac.h
  // https://chromium.googlesource.com/chromium/src/+/HEAD/device/gamepad/gamepad_device_mac.mm
  struct GamepadRef* gamepad = NULL;

  /*
  CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device_ref, NULL, kIOHIDOptionsTypeNone);
  CFIndex num_elements = CFArrayGetCount(elements);
  for (CFIndex i = 0; i < num_elements; i++) {
      IOHIDElementRef element = (IOHIDElementRef) CFArrayGetValueAtIndex(elements, i);
      if (!check_collection(element)) continue;
      uint32_t usage_page = IOHIDElementGetUsagePage(element);
      uint32_t usage = IOHIDElementGetUsage(element);
      if (IOHIDElementGetType(element) == kIOHIDElementTypeInput_Button && usage_page == kButtonUsagePage) {
        switch (usage) {
            case kHIDUsage_GD_DPadUp:
                break;
            case kHIDUsage_GD_DPadRight:
                break;
            case kHIDUsage_GD_DPadDown:
                break;
            case kHIDUsage_GD_DPadLeft:
                break;
            case kHIDUsage_GD_Y:
                break;
            case kHIDUsage_GD_X:
                break;
            case kHIDUsage_GD_Y:
                break;
            case kHIDUsage_GD_X:
                break;
            case kHIDUsage_GD_Y:
                break;
            default:
                printf("Unhandle usage: %#04x\n", usage);
                break;
        }
        uint32_t button_index = usage - 1;
        if (button_index < Gamepad::kButtonsLengthCap) {
          button_elements[button_index] = element;
          gamepad->buttons_length = MAX(gamepad->buttons_length, button_index + 1);
        }
      } else if (IOHIDElementGetType(element) == kIOHIDElementTypeInput_Misc) {
        uint32_t axis_index = usage - kAxisMinimumUsageNumber;
        if (axis_index < Gamepad::kAxesLengthCap && !axis_elements_[axis_index]) {
          axis_elements_[axis_index] = element;
          gamepad->axes_length = MAX(gamepad->axes_length, axis_index + 1);
        } else {
          mapped_all_axes = false;
        }
      }
  }
  */

  // Setup force feedback.
  gamepad->ff_device_ref = CreateForceFeedbackDevice(device_ref);
  if (gamepad->ff_device_ref) {
      gamepad->ff_effect_ref = CreateForceFeedbackEffect(
              gamepad->ff_device_ref,
              &gamepad->ff_effect,
              &gamepad->ff_custom_force,
              gamepad->force_data,
              gamepad->axes_data,
              gamepad->direction_data
      );
  }

}

static void gamepadWasRemoved(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef device) {
  printf("Gamepad was unplugged\n");
}

static void gamepad_input_value_callback(void* inContext, IOReturn inResult, void* inSender, IOHIDValueRef value) {
  printf("Gamepad talked!\n");

  uint32_t value_length = IOHIDValueGetLength(value);
  if (value_length > 4) {
    // Workaround for bizarre issue with PS3 controllers that try to return
    // massive (30+ byte) values and crash IOHIDValueGetIntegerValue
    return;
  }

  IOHIDElementRef element = IOHIDValueGetElement(value);
  uint32_t usage_page = IOHIDElementGetUsagePage(element);
  uint32_t usage = IOHIDElementGetUsage(element);
  IOHIDElementType element_type = IOHIDElementGetType(element);
  if (element_type == kIOHIDElementTypeInput_Button && usage_page == kButtonUsagePage) {
      bool pressed = IOHIDValueGetIntegerValue(value);
      printf("Pressed? %s\n", pressed ? "true" : "false");
      //gamepad->timestamp = MAX(gamepad->timestamp, IOHIDValueGetTimeStamp(value));
      switch (usage) {
            case kHIDUsage_GD_DPadUp:
                printf("DPAD UP\n");
                break;
            case kHIDUsage_GD_DPadRight:
                printf("DPAD RIGHT\n");
                break;
            case kHIDUsage_GD_DPadDown:
                printf("DPAD DOWN\n");
                break;
            case kHIDUsage_GD_DPadLeft:
                printf("DPAD LEFT\n");
                break;
            case kHIDUsage_GD_Y:
                printf("GD_Y\n");
                break;
            case kHIDUsage_GD_X:
                printf("GD_X\n");
                break;
            case kHIDUsage_GD_Z:
                printf("GD_Z\n");
                break;
            default:
                printf("Unhandle usage: %#04x\n", usage);
                break;
        }
  } else if (element_type == kIOHIDElementTypeInput_Misc && usage >= kAxisMinimumUsageNumber) {
      CFIndex axis_value = IOHIDValueGetIntegerValue(value);
      uint32_t report_size = IOHIDElementGetReportSize(element);
      printf("Axis: usage = %d, value = %d\n", usage, (int) axis_value);
      CFIndex axis_min = IOHIDElementGetLogicalMin(element);
      CFIndex axis_max = IOHIDElementGetLogicalMax(element);
      float value;
      if (axis_min > axis_max) {
          switch (report_size) {
            case 8:
              value = NormalizeUInt8Axis(axis_value, axis_min, axis_max);
              break;
            case 16:
              value = NormalizeUInt16Axis(axis_value, axis_min, axis_max);
              break;
            case 32:
              value = NormalizeUInt32Axis(axis_value, axis_min, axis_max);
              break;
          }
      } else {
          value = NormalizeAxis(axis_value, axis_min, axis_max);
      }
      printf("Float value: %f\n", value);
  }
}

void setupGamepad(void* callback_context) {
  IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

  /*
  NSMutableDictionary* criterion = [[NSMutableDictionary alloc] init];
  [criterion setObject: [NSNumber numberWithInt: kHIDPage_GenericDesktop] forKey: (NSString*)CFSTR(kIOHIDDeviceUsagePageKey)];
  [criterion setObject: [NSNumber numberWithInt: kHIDUsage_GD_GamePad] forKey: (NSString*)CFSTR(kIOHIDDeviceUsageKey)];
  */
  // For C equivalent of above, see https://stackoverflow.com/questions/17367414/creating-cfdictionaryref-with-cfdictionarycreate-and-special-characters
  //const void* keys[] = { kIOHIDDeviceUsagePageKey, kIOHIDDeviceUsageKey };
  //const void* values[] = { kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad };

  uint32_t page = kHIDPage_GenericDesktop;
  CFNumberRef pageNumRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page);
  assert(pageNumRef);

  uint32_t usage = kHIDUsage_GD_GamePad;
  CFNumberRef usageNumRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
  assert(usageNumRef);

  const void *keys[2] = { CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey) };
  const void *values[2] = { pageNumRef, usageNumRef };
  CFDictionaryRef criterion = CFDictionaryCreate(NULL, keys, values, 2, NULL, NULL);

  IOHIDManagerSetDeviceMatching(hidManager, criterion);

  CFRelease(pageNumRef);
  CFRelease(usageNumRef);
  CFRelease(criterion);

  IOHIDManagerRegisterDeviceMatchingCallback(hidManager, gamepad_added_callback, (void*)callback_context);
  IOHIDManagerRegisterDeviceRemovalCallback(hidManager, gamepadWasRemoved, (void*)callback_context);
  IOHIDManagerRegisterInputValueCallback(hidManager, gamepad_input_value_callback, (void*)callback_context);

  IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

  IOReturn _tIOReturn = IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
  //IOHIDManagerRegisterInputValueCallback(hidManager, gamepadAction, (void*)callback_context);
}
