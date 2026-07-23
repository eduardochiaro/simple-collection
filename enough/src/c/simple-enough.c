#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static GDrawCommandImage *s_number_6;

// Time tracking
static struct tm s_last_time;

// Settings
static bool s_invert_colors = false;

// Color helper functions
static GColor get_background_color() {
  return s_invert_colors ? GColorBlack : GColorWhite;
}

static GColor get_line_color() {
  return s_invert_colors ? PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite) : PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack);;
}

static GColor get_accent_color() {
  return s_invert_colors ? GColorWhite : GColorBlack;
}

static GColor get_hand_hour_color() {
  #ifdef PBL_COLOR
    return GColorDarkCandyAppleRed;  // Always red on color screens
  #else
    return s_invert_colors ? GColorWhite : GColorBlack;  // Reverse on b/w screens
  #endif
}
static GColor get_hand_minute_color() {
  #ifdef PBL_COLOR
    return GColorRed;  // Always red on color screens
  #else
    return s_invert_colors ? GColorWhite : GColorBlack;  // Reverse on b/w screens
  #endif
}

// Recolor a PDC image in place (numbers are stroke-only paths)
static void pdc_set_stroke_color(GDrawCommandImage *img, GColor color) {
  GDrawCommandList *list = gdraw_command_image_get_command_list(img);
  for (uint32_t i = 0, n = gdraw_command_list_get_num_commands(list); i < n; i++) {
    gdraw_command_set_stroke_color(gdraw_command_list_get_command(list, i), color);
  }
}

// The number PDCs are drawn for chalk's 180px round screen. Bigger round
// screens (gabbro, 260px) get them scaled up in place at load time.
#if defined(PBL_ROUND) && PBL_DISPLAY_WIDTH > 180
  #define PDC_SCALE_NUM PBL_DISPLAY_WIDTH
  #define PDC_SCALE_DEN 180
  #define HAND_WIDTH 5       // thicker hands to match the bigger screen
  #define HOUR_HAND_PCT 75   // shorter hour hand, the scaled-up face makes it read long
  #define ACCENT_LINE_LEN 65 // 12/3/9 o'clock marks, scaled from 45

static void pdc_scale(GDrawCommandImage *img) {
  GDrawCommandList *list = gdraw_command_image_get_command_list(img);
  for (uint32_t i = 0, n = gdraw_command_list_get_num_commands(list); i < n; i++) {
    GDrawCommand *cmd = gdraw_command_list_get_command(list, i);
    for (uint16_t p = 0, np = gdraw_command_get_num_points(cmd); p < np; p++) {
      GPoint pt = gdraw_command_get_point(cmd, p);
      gdraw_command_set_point(cmd, p, GPoint(pt.x * PDC_SCALE_NUM / PDC_SCALE_DEN,
                                             pt.y * PDC_SCALE_NUM / PDC_SCALE_DEN));
    }
    gdraw_command_set_stroke_width(cmd, gdraw_command_get_stroke_width(cmd) * PDC_SCALE_NUM / PDC_SCALE_DEN);
  }
  GSize size = gdraw_command_image_get_bounds_size(img);
  gdraw_command_image_set_bounds_size(img, GSize(size.w * PDC_SCALE_NUM / PDC_SCALE_DEN,
                                                 size.h * PDC_SCALE_NUM / PDC_SCALE_DEN));
}
#else
  #define pdc_scale(img) ((void)0)
  #define HAND_WIDTH 3
  #define HOUR_HAND_PCT 100
  #define ACCENT_LINE_LEN 45
#endif

// Load settings
static void load_settings() {
  s_invert_colors = persist_exists(MESSAGE_KEY_INVERT_COLORS) ? 
                    persist_read_bool(MESSAGE_KEY_INVERT_COLORS) : false;
}

// Save settings
static void save_settings() {
  persist_write_bool(MESSAGE_KEY_INVERT_COLORS, s_invert_colors);
}

// Inbox received callback
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *invert_tuple = dict_find(iterator, MESSAGE_KEY_INVERT_COLORS);
  
  if (invert_tuple) {
    s_invert_colors = invert_tuple->value->int32 == 1;
    save_settings();
    layer_mark_dirty(s_canvas_layer);
  }
}

// Drawing the clock face
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  // Set background
  graphics_context_set_fill_color(ctx, get_background_color());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw radial lines (12 segments)
  graphics_context_set_stroke_color(ctx, get_line_color());
  graphics_context_set_stroke_width(ctx, 1);
  
  // Use a large enough radius to ensure all lines reach edges
  int radius = bounds.size.w > bounds.size.h ? bounds.size.w : bounds.size.h;
  
  for (int i = 0; i < 12; i++) {
    int32_t angle = TRIG_MAX_ANGLE * i / 12;
    GPoint outer = {
      .x = (int16_t)(sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, center, outer);
  }
  
  // Draw thicker lines for 12, 3, and 9 o'clock (50px from center)
  graphics_context_set_stroke_color(ctx, get_accent_color());
  graphics_context_set_stroke_width(ctx, 2);
  
  // 12 o'clock (top)
  int32_t angle_12 = 0;
  GPoint line_12 = {
    .x = center.x,
    .y = (int16_t)(-cos_lookup(angle_12) * ACCENT_LINE_LEN / TRIG_MAX_RATIO) + center.y,
  };
  graphics_draw_line(ctx, center, line_12);
  
  // 3 o'clock (right)
  int32_t angle_3 = TRIG_MAX_ANGLE / 4;
  GPoint line_3 = {
    .x = (int16_t)(sin_lookup(angle_3) * ACCENT_LINE_LEN / TRIG_MAX_RATIO) + center.x,
    .y = center.y,
  };
  graphics_draw_line(ctx, center, line_3);
  
  // 9 o'clock (left)
  int32_t angle_9 = TRIG_MAX_ANGLE * 3 / 4;
  GPoint line_9 = {
    .x = (int16_t)(sin_lookup(angle_9) * ACCENT_LINE_LEN / TRIG_MAX_RATIO) + center.x,
    .y = center.y,
  };
  graphics_draw_line(ctx, center, line_9);
  
  // Draw white circle behind hands
  graphics_context_set_fill_color(ctx, get_background_color());
  graphics_fill_circle(ctx, center, 20);

  
  // Draw PDC number 6 at bottom
  const int top_padding = 22;
  if (s_number_6) {
    GSize img_size = gdraw_command_image_get_bounds_size(s_number_6);
    pdc_set_stroke_color(s_number_6, get_accent_color());

    // Draw background for number 6
    graphics_context_set_fill_color(ctx, get_background_color());
    graphics_fill_rect(ctx, GRect(center.x - (img_size.w - 4) / 2, (bounds.size.h / 2) + top_padding - 4, img_size.w - 4, img_size.h + 8), 2, GCornersAll);

    GRect img_rect = GRect(center.x - img_size.w / 2, (bounds.size.h / 2) + top_padding, img_size.w, img_size.h);
    gdraw_command_image_draw(ctx, s_number_6, img_rect.origin);
  }
  
  // Calculate time values
  int hour = s_last_time.tm_hour % 12;
  int minute = s_last_time.tm_min;
  
  // Calculate hand angles
  int32_t minute_angle = TRIG_MAX_ANGLE * minute / 60;
  int32_t hour_angle = (TRIG_MAX_ANGLE * ((hour * 60) + minute)) / (12 * 60);
  
  // Draw hour hand (shorter, thicker, red)
  graphics_context_set_stroke_width(ctx, HAND_WIDTH);
  graphics_context_set_stroke_color(ctx, get_hand_hour_color());
  GPoint hour_hand = {
    .x = (int16_t)(sin_lookup(hour_angle) * ((bounds.size.w / 2 - PBL_IF_RECT_ELSE(28, 44)) * HOUR_HAND_PCT / 100) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * ((bounds.size.w / 2 - PBL_IF_RECT_ELSE(28, 44)) * HOUR_HAND_PCT / 100) / TRIG_MAX_RATIO) + center.y,
  };
  GPoint hour_hand_tail = {
    .x = (int16_t)(-sin_lookup(hour_angle) * 16 / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(cos_lookup(hour_angle) * 16 / TRIG_MAX_RATIO) + center.y,
  };
  // Draw from tail through center to tip
  graphics_draw_line(ctx, hour_hand_tail, center);
  graphics_draw_line(ctx, center, hour_hand);
  
  // Draw minute hand (longer, medium thickness, red)
  graphics_context_set_stroke_width(ctx, HAND_WIDTH);
  graphics_context_set_stroke_color(ctx, get_hand_minute_color());
  GPoint minute_hand = {
    .x = (int16_t)(sin_lookup(minute_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(8, 22)) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(8, 22)) / TRIG_MAX_RATIO) + center.y,
  };
  GPoint minute_hand_tail = {
    .x = (int16_t)(-sin_lookup(minute_angle) * 16 / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(cos_lookup(minute_angle) * 16 / TRIG_MAX_RATIO) + center.y,
  };
  // Draw from tail through center to tip
  graphics_draw_line(ctx, minute_hand_tail, center);
  graphics_draw_line(ctx, center, minute_hand);
  
  // Draw center circle with red border
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_circle(ctx, center, 4);
  graphics_context_set_fill_color(ctx, GColorWhite);
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
  
  // Load NUMBER_6 PDC resource
  s_number_6 = gdraw_command_image_create_with_resource(RESOURCE_ID_NUMBER_6);
  pdc_scale(s_number_6);
  
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
  gdraw_command_image_destroy(s_number_6);
}

// App initialization
static void init() {
  // Load settings
  load_settings();
  
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
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(128, 128);
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
