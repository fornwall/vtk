// See https://wayland-book.com/seat.html and https://wayland-book.com/seat/example.html

#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "vtk_log.h"
#include "vtk_cffi.h"
#include "vtk_wayland_keyboard.h"

static void vtk_wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd,
                               uint32_t size) {
  LOGI("vtk_wl_keyboard called: size = %d, fd = %d", size, fd);
  struct VtkContextNative* context = (struct VtkContextNative*) data;
  assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

  void* keymap_mem = malloc(size);
  assert(keymap_mem != NULL);
  assert(read(fd, keymap_mem, size) == size);
  close(fd);
  //char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  //if (map_shm == MAP_FAILED) { perror("mmap"); }
  //assert(map_shm != MAP_FAILED);

  struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(context->wayland_xkb_context, keymap_mem,
                                                             XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  free(keymap_mem);


  struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);

  xkb_keymap_unref(context->wayland_xkb_keymap);
  context->wayland_xkb_keymap = xkb_keymap;

  xkb_state_unref(context->wayland_xkb_state);
  context->wayland_xkb_state = xkb_state;
}

static void vtk_wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface,
                              struct wl_array *keys) {
  struct VtkContextNative* context = (struct VtkContextNative*) data;
  fprintf(stderr, "keyboard enter; keys pressed are:\n");
  uint32_t *key;
  wl_array_for_each(key, keys) {
    char buf[128];
    xkb_keysym_t sym = xkb_state_key_get_one_sym(context->wayland_xkb_state, *key + 8);
    xkb_keysym_get_name(sym, buf, sizeof(buf));
    fprintf(stderr, "sym: %-12s (%d), ", buf, sym);
    xkb_state_key_get_utf8(context->wayland_xkb_state, *key + 8, buf, sizeof(buf));
    fprintf(stderr, "utf8: '%s'\n", buf);
  }
}

static void vtk_wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key,
                            uint32_t state) {
  struct VtkContextNative* context = (struct VtkContextNative*) data;
  char buf[128];
  uint32_t keycode = key + 8;
  xkb_keysym_t sym = xkb_state_key_get_one_sym(context->wayland_xkb_state, keycode);
  xkb_keysym_get_name(sym, buf, sizeof(buf));
  const char *action = state == WL_KEYBOARD_KEY_STATE_PRESSED ? "press" : "release";
  fprintf(stderr, "key %s: sym: %-12s (%d), ", action, buf, sym);
  xkb_state_key_get_utf8(context->wayland_xkb_state, keycode, buf, sizeof(buf));
  fprintf(stderr, "utf8: '%s'\n", buf);
}

static void vtk_wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial,
                              struct wl_surface *surface) {
  fprintf(stderr, "keyboard leave\n");
}

static void vtk_wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t mods_depressed,
                                  uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
  struct VtkContextNative* context = (struct VtkContextNative*) data;
  xkb_state_update_mask(context->wayland_xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

static void vtk_wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay) {
  /* Left as an exercise for the reader */
}

static const struct wl_keyboard_listener vtk_wl_keyboard_listener = {
    .keymap = vtk_wl_keyboard_keymap,
    .enter = vtk_wl_keyboard_enter,
    .leave = vtk_wl_keyboard_leave,
    .key = vtk_wl_keyboard_key,
    .modifiers = vtk_wl_keyboard_modifiers,
    .repeat_info = vtk_wl_keyboard_repeat_info,
};

void vtk_wayland_add_keyboard_listener(struct VtkContextNative* vtk_context) {
    assert(vtk_context->wayland_keyboard == NULL);
    assert(vtk_context->wayland_seat != NULL);
    vtk_context->wayland_keyboard = wl_seat_get_keyboard(vtk_context->wayland_seat);
    assert(vtk_context->wayland_keyboard != NULL);
    wl_keyboard_add_listener(vtk_context->wayland_keyboard, &vtk_wl_keyboard_listener, vtk_context);
}
