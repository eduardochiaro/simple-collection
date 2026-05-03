#include <pebble.h>
#include "face_common.h"
#include "face_round.h"
#include "face_rect.h"

static Window *s_main_window;
static Layer *s_canvas_layer;
static BitmapLayer *s_text_layer;
static GBitmap *s_text_bitmap;

// Time tracking
static struct tm s_last_time;

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

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  int circle_r = bounds.size.w / 2 - PBL_IF_RECT_ELSE(10, 16);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Pre-rotated "pebble" text bitmap
  if (bounds.size.w >= 200) {
    s_text_bitmap = gbitmap_create_with_resource(TEXT_PEBBLE_LARGE_RESOURCE);
  } else {
    s_text_bitmap = gbitmap_create_with_resource(TEXT_PEBBLE_RESOURCE);
  }
  GSize img_size = gbitmap_get_bounds(s_text_bitmap).size;
  int lx = circle_r / 2;
  // On large screens band_half=40, so push label above the band edge
  int ly = (bounds.size.w >= 200) ? -58 : -38;
  GPoint dest = {
    .x = (int16_t)(lx * cos_lookup(ROTATION_OFFSET) / TRIG_MAX_RATIO
                 - ly * sin_lookup(ROTATION_OFFSET) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(lx * sin_lookup(ROTATION_OFFSET) / TRIG_MAX_RATIO
                 + ly * cos_lookup(ROTATION_OFFSET) / TRIG_MAX_RATIO) + center.y,
  };
  GRect img_frame = GRect(dest.x - img_size.w / 2, dest.y - img_size.h / 2,
                          img_size.w, img_size.h);
  s_text_layer = bitmap_layer_create(img_frame);
  bitmap_layer_set_bitmap(s_text_layer, s_text_bitmap);
  bitmap_layer_set_compositing_mode(s_text_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_text_layer));

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
  bitmap_layer_destroy(s_text_layer);
  gbitmap_destroy(s_text_bitmap);
}

// Main
int main(void) {
  init();
  app_event_loop();
  deinit();
}
