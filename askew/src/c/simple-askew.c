#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static BitmapLayer *s_text_layer;
static GBitmap *s_text_bitmap;

// Time tracking
static struct tm s_last_time;

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

// Rotation offset: rotate entire face clockwise by ANGLE degrees
#define ANGLE 25
#define ROTATION_OFFSET (TRIG_MAX_ANGLE * ANGLE / 360)

#if ANGLE == 10
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_10 
#elif ANGLE == 15
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_15
#elif ANGLE == 20
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_20
#elif ANGLE == 25
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_25
#elif ANGLE == 30
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_30
#else
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_10
#endif

// Drawing the clock face
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  // Set background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Use a large enough radius to ensure all lines reach edges
  int radius = bounds.size.w > bounds.size.h ? bounds.size.w : bounds.size.h;

  int circle_offset = PBL_IF_RECT_ELSE(10, 16);
  // Draw circle behind hands
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, bounds.size.w / 2 - circle_offset);

  // Draw DarkGray band (50px tall) spanning full circle width, rotated by ROTATION_OFFSET
  int circle_r = bounds.size.w / 2 - circle_offset;
  int band_half_height = bounds.size.w >= 200 ? 40 : 25;
  GPoint band_points[4] = {
    { -bounds.size.w, -band_half_height},
    { bounds.size.w, -band_half_height},
    { bounds.size.w,  band_half_height},
    { -bounds.size.w,  band_half_height},
  };
  GPathInfo band_info = {
    .num_points = 4,
    .points = band_points,
  };
  GPath *band_path = gpath_create(&band_info);
  gpath_rotate_to(band_path, ROTATION_OFFSET);
  gpath_move_to(band_path, center);
  graphics_context_set_fill_color(ctx, GColorLightGray);
  gpath_draw_filled(ctx, band_path);
  gpath_destroy(band_path);

  // Mask band overflow: single thick arc outside the circle
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 30);
  int arc_r = circle_r + 15;
  GRect arc_rect = GRect(center.x - arc_r, center.y - arc_r, arc_r * 2, arc_r * 2);
  graphics_draw_arc(ctx, arc_rect, GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE);

  // draw radial smaller lines (60 segments)
  graphics_context_set_stroke_color(ctx, GColorWindsorTan);
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 0; i < 60; i++) {
    if (i % 5 == 0) continue; // Skip the hour lines
    int32_t angle = TRIG_MAX_ANGLE * i / 60 + ROTATION_OFFSET;
    GPoint inner = {
      .x = (int16_t)(sin_lookup(angle) * (circle_r + 2) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * (circle_r + 2) / TRIG_MAX_RATIO) + center.y,
    };
    GPoint outer = {
      .x = (int16_t)(sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, inner, outer);
  }

  // Draw radial lines (12 segments)
  graphics_context_set_stroke_color(ctx, GColorDarkCandyAppleRed);
  graphics_context_set_stroke_width(ctx, 3);
  for (int i = 0; i < 12; i++) {
    int32_t angle = TRIG_MAX_ANGLE * i / 12 + ROTATION_OFFSET;
    GPoint inner = {
      .x = (int16_t)(sin_lookup(angle) * (circle_r + 2) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * (circle_r + 2) / TRIG_MAX_RATIO) + center.y,
    };
    GPoint outer = {
      .x = (int16_t)(sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, inner, outer);
  }

  // Calculate time values
  int hour = s_last_time.tm_hour % 12;
  int minute = s_last_time.tm_min;
  
  // Calculate hand angles
  int32_t minute_angle = TRIG_MAX_ANGLE * minute / 60 + ROTATION_OFFSET;
  int32_t hour_angle = (TRIG_MAX_ANGLE * ((hour * 60) + minute)) / (12 * 60) + ROTATION_OFFSET;
  
  // Draw hour hand (shorter, thicker, red)
  graphics_context_set_stroke_width(ctx, 4);
  graphics_context_set_stroke_color(ctx, get_hand_hour_color());
  int hour_hand_offset = bounds.size.w >= 201 ? 64 : PBL_IF_RECT_ELSE(36, 44);
  GPoint hour_hand = {
    .x = (int16_t)(sin_lookup(hour_angle) * (bounds.size.w / 2 - hour_hand_offset) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (bounds.size.w / 2 - hour_hand_offset) / TRIG_MAX_RATIO) + center.y,
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
  GPoint center = grect_center_point(&bounds);
  int circle_r = bounds.size.w / 2 - PBL_IF_RECT_ELSE(10, 16);

  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Create text bitmap layer from pre-rotated PNG resource
  s_text_bitmap = gbitmap_create_with_resource(TEXT_PEBBLE_RESOURCE);
  GSize img_size = gbitmap_get_bounds(s_text_bitmap).size;
  // Position: above right side of band in rotated coordinate frame
  int lx = circle_r / 2;
  int ly = -38;
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

  // Get initial time
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
