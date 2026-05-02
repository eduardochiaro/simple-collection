#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;

// Time tracking
static struct tm s_last_time;


// Color helper functions
static GColor get_background_color() {
  return GColorWhite;
}

static GColor get_line_color() {
  return PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack);
}

static GColor get_accent_color() {
  return GColorBlack;
}

static GColor get_hand_hour_color() {
  #ifdef PBL_COLOR
    return GColorChromeYellow;  // Always red on color screens
  #else
    return GColorBlack;  // Always black on b/w screens
  #endif
}
static GColor get_hand_minute_color() {
  #ifdef PBL_COLOR
    return GColorRajah;  // Always red on color screens
  #else
    return GColorBlack;  // Always black on b/w screens
  #endif
}

// Drawing the clock face
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  // Set background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Use a large enough radius to ensure all lines reach edges
  int radius = bounds.size.w > bounds.size.h ? bounds.size.w : bounds.size.h;

  // draw radial smaller lines (60 segments)
  graphics_context_set_stroke_color(ctx, GColorWindsorTan);
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 0; i < 60; i++) {
    if (i % 5 == 0) continue; // Skip the hour lines
    int32_t angle = TRIG_MAX_ANGLE * i / 60;
    GPoint outer = {
      .x = (int16_t)(sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, center, outer);
  }
  
  // Draw radial lines (12 segments)
  graphics_context_set_stroke_color(ctx, GColorWindsorTan);
  graphics_context_set_stroke_width(ctx, 2);
  
  for (int i = 0; i < 12; i++) {
    int32_t angle = TRIG_MAX_ANGLE * i / 12;
    GPoint outer = {
      .x = (int16_t)(sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, center, outer);
  }
  
  // Draw white circle behind hands
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_circle(ctx, center, bounds.size.w / 2 - 16);

  
  // Calculate time values
  int hour = s_last_time.tm_hour % 12;
  int minute = s_last_time.tm_min;
  
  // Calculate hand angles
  int32_t minute_angle = TRIG_MAX_ANGLE * minute / 60;
  int32_t hour_angle = (TRIG_MAX_ANGLE * ((hour * 60) + minute)) / (12 * 60);
  
  // Draw hour hand (shorter, thicker, red)
  graphics_context_set_stroke_width(ctx, 4);
  graphics_context_set_stroke_color(ctx, get_hand_hour_color());
  GPoint hour_hand = {
    .x = (int16_t)(sin_lookup(hour_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(36, 44)) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(36, 44)) / TRIG_MAX_RATIO) + center.y,
  };
  GPoint hour_hand_tail = {
    .x = (int16_t)(-sin_lookup(hour_angle) * 16 / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(cos_lookup(hour_angle) * 16 / TRIG_MAX_RATIO) + center.y,
  };
  // Draw from tail through center to tip
  graphics_draw_line(ctx, hour_hand_tail, center);
  graphics_draw_line(ctx, center, hour_hand);
  
  // Draw minute hand (longer, medium thickness, red)
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, get_hand_minute_color());
  GPoint minute_hand = {
    .x = (int16_t)(sin_lookup(minute_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(20, 22)) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(20, 22)) / TRIG_MAX_RATIO) + center.y,
  };
  GPoint minute_hand_tail = {
    .x = (int16_t)(-sin_lookup(minute_angle) * 16 / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(cos_lookup(minute_angle) * 16 / TRIG_MAX_RATIO) + center.y,
  };
  // Draw from tail through center to tip
  graphics_draw_line(ctx, minute_hand_tail, center);
  graphics_draw_line(ctx, center, minute_hand);
  
  // Draw center circle with red border
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_circle(ctx, center, 4);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 3);
}

// Update time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_last_time = *tick_time;
  layer_mark_dirty(s_canvas_layer);
}

// Window load
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  // Get initial time
  time_t temp = time(NULL);
  s_last_time = *localtime(&temp);
}

// Window unload
static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

// App initialization
static void init() {
  
  // Create main window
  s_main_window = window_create();
  window_set_background_color(s_main_window, get_background_color());
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
