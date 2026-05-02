#include <pebble.h>

static int isqrt(int n) {
  if (n <= 0) return 0;
  int x = n, y = 1;
  while (x > y) { x = (x + y) / 2; y = n / x; }
  return x;
}

static Window *s_main_window;
static Layer *s_canvas_layer;

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

// Rotation offset: rotate entire face clockwise by 15 degrees
#define ANGLE 10
#define ROTATION_OFFSET (TRIG_MAX_ANGLE * ANGLE / 360)

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
  graphics_context_set_fill_color(ctx, GColorLightGray);
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
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  gpath_draw_filled(ctx, band_path);
  gpath_destroy(band_path);

  // Mask band overflow: draw black ring outside the circle
  graphics_context_set_stroke_color(ctx, GColorBlack);
  for (int r = circle_r + 1; r <= circle_r + 30; r++) {
    graphics_draw_circle(ctx, center, r);
  }

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

  // Draw "pebble" text just above right side of the band
  // Local coords: (circle_r/2, -37) = right half, above the band top edge
  {
    int lx = circle_r / 2;
    int ly = -37;
    GPoint text_pos = {
      .x = (int16_t)(lx * cos_lookup(ROTATION_OFFSET) / TRIG_MAX_RATIO
                   + ly * sin_lookup(ROTATION_OFFSET) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-lx * sin_lookup(ROTATION_OFFSET) / TRIG_MAX_RATIO
                   + ly * cos_lookup(ROTATION_OFFSET) / TRIG_MAX_RATIO) + center.y,
    };
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    GRect text_box = GRect(text_pos.x - 25, text_pos.y - 8, 50, 16);
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, "pebble", font, text_box,
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
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
