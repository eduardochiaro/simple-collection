#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;

static GPoint s_center;
static int s_radius;
static int s_hour_hand_length = 60;
static int s_minute_hand_length = 40;
static int s_hover_hand_length = 20;
static GColor s_background_color;
static GColor s_hours_color;
static GColor s_minutes_color;
static bool s_use_rect;

// Calculate the angle for hour hand (0 = 12 o'clock, clockwise)
static int32_t get_hour_angle(struct tm *tick_time) {
  // Convert to 24-hour based angle
  // 0 hours = 0 degrees, 12 hours = 180 degrees, 24 hours = 360 degrees
  int hour = tick_time->tm_hour;
  if (hour >= 12) {
    hour -= 12;
  }
  int minute = tick_time->tm_min;
  
  // Calculate precise angle including minutes
  return TRIG_MAX_ANGLE / 12 * hour + (TRIG_MAX_ANGLE / 12) * minute / 60;
}

// Calculate the angle for minute hand (0 = 12 o'clock, clockwise)
static int32_t get_minute_angle(struct tm *tick_time) {
  return TRIG_MAX_ANGLE * tick_time->tm_min / 60;
}

// Load settings
static void load_settings() {
  s_use_rect = persist_exists(MESSAGE_KEY_USE_RECT) ? persist_read_bool(MESSAGE_KEY_USE_RECT) : false;
  s_background_color = persist_exists(MESSAGE_KEY_BACKGROUND_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_BACKGROUND_COLOR) } : GColorWhite;
  s_hours_color = persist_exists(MESSAGE_KEY_HOURS_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_HOURS_COLOR) } : GColorBlack;
  s_minutes_color = persist_exists(MESSAGE_KEY_MINUTES_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_MINUTES_COLOR) } : GColorBlack;
}

// Save settings
static void save_settings() {
  persist_write_bool(MESSAGE_KEY_USE_RECT, s_use_rect);
  persist_write_int(MESSAGE_KEY_BACKGROUND_COLOR, s_background_color.argb);
  persist_write_int(MESSAGE_KEY_HOURS_COLOR, s_hours_color.argb);
  persist_write_int(MESSAGE_KEY_MINUTES_COLOR, s_minutes_color.argb);
}

// Inbox received callback
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *rect_tuple = dict_find(iterator, MESSAGE_KEY_USE_RECT);
  bool changed = false;
  
  if (rect_tuple) {
    s_use_rect = rect_tuple->value->int32 == 1;
    changed = true;
  }

  Tuple *bgcolor_tuple = dict_find(iterator, MESSAGE_KEY_BACKGROUND_COLOR);
  if (bgcolor_tuple) {
    s_background_color = GColorFromHEX(bgcolor_tuple->value->int32);
    changed = true;
  }

  Tuple *hourscolor_tuple = dict_find(iterator, MESSAGE_KEY_HOURS_COLOR);
  if (hourscolor_tuple) {
    s_hours_color = GColorFromHEX(hourscolor_tuple->value->int32);
    changed = true;
  }

  Tuple *minutescolor_tuple = dict_find(iterator, MESSAGE_KEY_MINUTES_COLOR);
  if (minutescolor_tuple) {
    s_minutes_color = GColorFromHEX(minutescolor_tuple->value->int32);
    changed = true;
  }

  save_settings();

  if (changed) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static GColor reverse_color(GColor color) {
  // Simple inversion for black and white
  if (color.argb == GColorBlack.argb) {
    return GColorWhite;
  } else {
    return GColorBlack;
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  s_center = grect_center_point(&bounds);
  
  // Calculate radius to fit in the display
  s_radius = !s_use_rect ? ((bounds.size.w - 2) / 2) : ((bounds.size.h) / 2 + 40);
  
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  
  int32_t hour_angle = get_hour_angle(tick_time);
  int32_t minute_angle = get_minute_angle(tick_time);
  int hour = tick_time->tm_hour;
  
  // Fill the background white
  graphics_context_set_fill_color(ctx, s_background_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw hour hand (from border to center, starting at s_hour_hand_length from center)
  graphics_context_set_stroke_color(ctx, s_hours_color);
  graphics_context_set_stroke_width(ctx, 4);
  
  int32_t sin_hour_val = sin_lookup(hour_angle);
  int32_t cos_hour_val = cos_lookup(hour_angle);
  
  GPoint hour_start = (GPoint){
    .x = s_center.x + (sin_hour_val * s_radius / TRIG_MAX_RATIO),
    .y = s_center.y - (cos_hour_val * s_radius / TRIG_MAX_RATIO)
  };
  
  GPoint hour_end = (GPoint){
    .x = s_center.x + (sin_hour_val * s_hour_hand_length / TRIG_MAX_RATIO),
    .y = s_center.y - (cos_hour_val * s_hour_hand_length / TRIG_MAX_RATIO)
  };
  
  graphics_draw_line(ctx, hour_start, hour_end);
  
  // Draw white inner stroke for hour hand (from hour_end, 10px toward border)
  graphics_context_set_stroke_color(ctx, GColorMalachite);
  graphics_context_set_stroke_width(ctx, 2);
  
  GPoint hour_white_end = (GPoint){
    .x = s_center.x + (sin_hour_val * (s_hour_hand_length + s_hover_hand_length) / TRIG_MAX_RATIO),
    .y = s_center.y - (cos_hour_val * (s_hour_hand_length + s_hover_hand_length) / TRIG_MAX_RATIO)
  };
  
  graphics_draw_line(ctx, hour_end, hour_white_end);
  
  // Draw minute hand (from border to center, starting at s_minute_hand_length from center)
  graphics_context_set_stroke_color(ctx, s_minutes_color);
  graphics_context_set_stroke_width(ctx, 4);
  
  int32_t sin_val = sin_lookup(minute_angle);
  int32_t cos_val = cos_lookup(minute_angle);
  
  GPoint minute_start = (GPoint){
    .x = s_center.x + (sin_val * s_radius / TRIG_MAX_RATIO),
    .y = s_center.y - (cos_val * s_radius / TRIG_MAX_RATIO)
  };
  
  GPoint minute_end = (GPoint){
    .x = s_center.x + (sin_val * s_minute_hand_length / TRIG_MAX_RATIO),
    .y = s_center.y - (cos_val * s_minute_hand_length / TRIG_MAX_RATIO)
  };
  
  graphics_draw_line(ctx, minute_start, minute_end);
  
  // Draw white inner stroke for minute hand (from minute_end, 10px toward border)
  graphics_context_set_stroke_color(ctx, GColorMalachite);
  graphics_context_set_stroke_width(ctx, 2);
  
  GPoint minute_white_end = (GPoint){
    .x = s_center.x + (sin_val * (s_minute_hand_length + s_hover_hand_length) / TRIG_MAX_RATIO),
    .y = s_center.y - (cos_val * (s_minute_hand_length + s_hover_hand_length) / TRIG_MAX_RATIO)
  };
  
  graphics_draw_line(ctx, minute_end, minute_white_end);
  
  // Draw border
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorLightGray, reverse_color(s_background_color)));
  graphics_context_set_stroke_width(ctx, 2);
  
  if (!s_use_rect) {
    // Round screen: draw circle border
    graphics_draw_circle(ctx, s_center, s_radius);
  } else {
    // Rectangular screen: draw 2px border following screen edge
    // Draw two rectangles to create a 2px border
    graphics_draw_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h));
    graphics_draw_rect(ctx, GRect(1, 1, bounds.size.w - 2, bounds.size.h - 2));
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Redraw the canvas
  layer_mark_dirty(s_canvas_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

static void init() {
  // Load settings
  load_settings();
  
  // Create main window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(128, 128);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
