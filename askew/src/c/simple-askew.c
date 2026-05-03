#include <pebble.h>
#include "face_common.h"
#include "face_round.h"
#include "face_rect.h"

#define PERSIST_KEY_ANGLE 0

// Runtime rotation offset — defined here, declared extern in face_common.h
int32_t g_rotation_offset;

static Window *s_main_window;
static Layer *s_canvas_layer;
static BitmapLayer *s_text_layer;
static GBitmap *s_text_bitmap;
static int s_current_angle;

// Time tracking
static struct tm s_last_time;

// (Re)load the text bitmap for the given angle and reposition the layer
static void set_text_bitmap(int angle) {
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  int circle_r = bounds.size.w / 2 - PBL_IF_RECT_ELSE(10, 16);
  bool large = bounds.size.w >= 200;

  if (s_text_bitmap) {
    gbitmap_destroy(s_text_bitmap);
  }
  s_text_bitmap = gbitmap_create_with_resource(get_text_resource_id(angle, large));
  GSize img_size = gbitmap_get_bounds(s_text_bitmap).size;

  int lx = circle_r / 2;
  int ly = large ? -58 : -38;
  GPoint dest = {
    .x = (int16_t)(lx * cos_lookup(g_rotation_offset) / TRIG_MAX_RATIO
                 - ly * sin_lookup(g_rotation_offset) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(lx * sin_lookup(g_rotation_offset) / TRIG_MAX_RATIO
                 + ly * cos_lookup(g_rotation_offset) / TRIG_MAX_RATIO) + center.y,
  };
  GRect img_frame = GRect(dest.x - img_size.w / 2, dest.y - img_size.h / 2,
                          img_size.w, img_size.h);
  layer_set_frame(bitmap_layer_get_layer(s_text_layer), img_frame);
  bitmap_layer_set_bitmap(s_text_layer, s_text_bitmap);
}

// Drawing the clock face
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
#ifdef PBL_ROUND
  face_round_draw(ctx, bounds, &s_last_time);
#else
  face_rect_draw(ctx, bounds, &s_last_time);
#endif
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_last_time = *tick_time;
  layer_mark_dirty(s_canvas_layer);
}

static void load_settings() {
  s_current_angle = persist_exists(PERSIST_KEY_ANGLE) ? persist_read_int(PERSIST_KEY_ANGLE) : DEFAULT_ANGLE;
}

static void save_settings() {
  persist_write_int(PERSIST_KEY_ANGLE, s_current_angle);
}

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_find(iter, MESSAGE_KEY_ANGLE);
  if (t) {
    int angle = (int)t->value->int32;
    s_current_angle = angle;
    g_rotation_offset = (int32_t)(TRIG_MAX_ANGLE * angle / 360);
    set_text_bitmap(angle);
    layer_mark_dirty(s_canvas_layer);
  }

  save_settings();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Create text layer (bitmap loaded below after frame is known)
  s_text_layer = bitmap_layer_create(GRect(0, 0, 1, 1));
  bitmap_layer_set_compositing_mode(s_text_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_text_layer));
  set_text_bitmap(s_current_angle);

  time_t temp = time(NULL);
  s_last_time = *localtime(&temp);
}

// Window unload
static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  bitmap_layer_destroy(s_text_layer);
  gbitmap_destroy(s_text_bitmap);
}

// App initialization
static void init() {
  // Load settings
  load_settings();
  g_rotation_offset = (int32_t)(TRIG_MAX_ANGLE * s_current_angle / 360);

  // Register AppMessage for Clay settings
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(128, 128);

  // Create main window
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

// App deinitialization
static void deinit() {
  window_destroy(s_main_window);
}

// Main
int main(void) {
  init();
  app_event_loop();
  deinit();
}
