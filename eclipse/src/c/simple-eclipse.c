#include <pebble.h>
#include <math.h>

// Define M_PI if not provided by the platform headers
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(__has_include)
#  if __has_include("message_keys.auto.h")
#    include "message_keys.auto.h"
#  endif
#endif

static Window *s_main_window;
static Layer *s_canvas_layer;
static bool s_invert_colors = false;
static bool s_use_square = false;
static GColor s_hand_color;

// Load settings
static void load_settings() {
  s_invert_colors = persist_exists(MESSAGE_KEY_INVERT_COLORS) ? 
                    persist_read_bool(MESSAGE_KEY_INVERT_COLORS) : false;
  s_use_square = persist_exists(MESSAGE_KEY_USE_SQUARE) ?
                 persist_read_bool(MESSAGE_KEY_USE_SQUARE) : false;
  if (persist_exists(MESSAGE_KEY_HOURS_COLOR)) {
    s_hand_color = (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_HOURS_COLOR) };
  } else {
    s_hand_color = GColorWhite;
  }
}

// Save settings
static void save_settings() {
  persist_write_bool(MESSAGE_KEY_INVERT_COLORS, s_invert_colors);
  persist_write_bool(MESSAGE_KEY_USE_SQUARE, s_use_square);
  persist_write_int(MESSAGE_KEY_HOURS_COLOR, s_hand_color.argb);
}

// AppMessage inbox handler for settings
static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  Tuple *inv_t = dict_find(iterator, MESSAGE_KEY_INVERT_COLORS);
  if (inv_t) {
    s_invert_colors = inv_t->value->int32 != 0;
    save_settings();
    layer_mark_dirty(s_canvas_layer);
  }
  Tuple *sq_t = dict_find(iterator, MESSAGE_KEY_USE_SQUARE);
  if (sq_t) {
    s_use_square = sq_t->value->int32 != 0;
    save_settings();
    layer_mark_dirty(s_canvas_layer);
  }
  Tuple *h_color_t = dict_find(iterator, MESSAGE_KEY_HOURS_COLOR);
  if (h_color_t) {
    s_hand_color = GColorFromHEX(h_color_t->value->int32);
    save_settings();
    layer_mark_dirty(s_canvas_layer); 
  }
}

// Draw a flat-ended rectangular marker using GPath
static void draw_marker(GContext *ctx, GPoint center, int32_t angle, int16_t inner_r, int16_t outer_r, int16_t thickness, GColor color) {
  int16_t hw = thickness / 2;

  // Rectangle from inner_r to outer_r along X-axis, centered vertically
  GPoint pts[4];
  pts[0] = GPoint(inner_r, -hw);
  pts[1] = GPoint(outer_r, -hw);
  pts[2] = GPoint(outer_r, hw);
  pts[3] = GPoint(inner_r, hw);

  GPathInfo path_info = { .num_points = 4, .points = pts };
  GPath *path = gpath_create(&path_info);
  gpath_rotate_to(path, angle);
  gpath_move_to(path, center);

  graphics_context_set_fill_color(ctx, color);
  gpath_draw_filled(ctx, path);

  gpath_destroy(path);
}

// Draw a marker with a border on the clockwise/right side only
static void draw_marker_with_border(GContext *ctx, GPoint center, int32_t angle, int16_t inner_r, int16_t outer_r, int16_t thickness, GColor fill_color, GColor border_color, int16_t border_width) {
  int16_t hw = thickness / 2;

  // Draw border strip on clockwise edge (local Y from hw -> hw+border_width)
  GPoint border_pts[4];
  border_pts[0] = GPoint(inner_r, hw);
  border_pts[1] = GPoint(outer_r, hw);
  border_pts[2] = GPoint(outer_r, hw + border_width);
  border_pts[3] = GPoint(inner_r, hw + border_width);

  GPathInfo border_info = { .num_points = 4, .points = border_pts };
  GPath *border_path = gpath_create(&border_info);
  gpath_rotate_to(border_path, angle);
  gpath_move_to(border_path, center);

  graphics_context_set_fill_color(ctx, border_color);
  gpath_draw_filled(ctx, border_path);
  gpath_destroy(border_path);

  // Draw fill on top
  draw_marker(ctx, center, angle, inner_r, outer_r, thickness, fill_color);
}

// Map colors when invert setting is enabled
static GColor map_color(GColor color) {
  if (!s_invert_colors) {
    return color;
  }
  if (color.argb == GColorBlack.argb) {
    return GColorWhite;
  }
  if (color.argb == GColorWhite.argb) {
    return GColorBlack;
  }
  if (color.argb == GColorDarkGray.argb) {
    return GColorLightGray;
  }
  if (color.argb == GColorLightGray.argb) {
    return GColorDarkGray;
  }
  return color;
}

// Compute distance from center along angle until the inset rectangle boundary is hit
// Uses Pebble's native fixed-point trig to avoid floating-point crashes
static int16_t radial_distance_to_inset(GRect bounds, int16_t inset, int32_t angle) {
  // Half extents reduced by inset
  int16_t hx = (bounds.size.w / 2) - inset;
  int16_t hy = (bounds.size.h / 2) - inset;
  if (hx < 1) hx = 1;
  if (hy < 1) hy = 1;

  // Get trig values using Pebble's lookup (returns value in range -TRIG_MAX_RATIO to TRIG_MAX_RATIO)
  int32_t cos_val = cos_lookup(angle);  // X component
  int32_t sin_val = sin_lookup(angle);  // Y component

  // Absolute values
  if (cos_val < 0) cos_val = -cos_val;
  if (sin_val < 0) sin_val = -sin_val;

  // Compute distance to each edge (avoid division by zero)
  // distance = half_extent * TRIG_MAX_RATIO / trig_val
  int32_t dist_x = 10000;
  int32_t dist_y = 10000;
  
  if (cos_val > 100) {
    dist_x = ((int32_t)hx * TRIG_MAX_RATIO) / cos_val;
  }
  if (sin_val > 100) {
    dist_y = ((int32_t)hy * TRIG_MAX_RATIO) / sin_val;
  }

  int32_t dist = (dist_x < dist_y) ? dist_x : dist_y;
  if (dist < 0) dist = 0;
  if (dist > 500) dist = 500;  // Reasonable max for watch screen
  return (int16_t)dist;
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

  // Ring parameters
  int16_t border = 2;
  #ifdef PBL_PLATFORM_EMERY
  // Thicker ring for larger Emery screen
  int16_t ring_thickness = 25;
  #else
  int16_t ring_thickness = PBL_IF_RECT_ELSE(15, 20);
  #endif

  // Decide mode: rectangular inset ring when setting enabled and device is rectangular
  bool rect_mode = s_use_square && PBL_IF_RECT_ELSE(true, false);

  if (rect_mode) {
    // Draw background
    graphics_context_set_fill_color(ctx, map_color(GColorBlack));
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    uint16_t corner_radius = 8; // rounded corners for inset rectangles

    // Outer dark gray border (full bounds)
    graphics_context_set_fill_color(ctx, map_color(GColorDarkGray));
    graphics_fill_rect(ctx, bounds, corner_radius, GCornersAll);

    // White ring outer rect (inset by border)
    GRect white_outer = GRect(bounds.origin.x + border, bounds.origin.y + border, bounds.size.w - border*2, bounds.size.h - border*2);
    graphics_context_set_fill_color(ctx, map_color(GColorWhite));
    graphics_fill_rect(ctx, white_outer, corner_radius, GCornersAll);

    // Inner dark gray border (inset by border + ring_thickness)
    GRect inner_border = GRect(bounds.origin.x + border + ring_thickness, bounds.origin.y + border + ring_thickness, bounds.size.w - 2*(border + ring_thickness), bounds.size.h - 2*(border + ring_thickness));
    graphics_context_set_fill_color(ctx, map_color(GColorDarkGray));
    graphics_fill_rect(ctx, inner_border, corner_radius, GCornersAll);

    // Center rect (inset further by border)
    GRect center_rect = GRect(bounds.origin.x + border + ring_thickness + border, bounds.origin.y + border + ring_thickness + border, bounds.size.w - 2*(border + ring_thickness + border), bounds.size.h - 2*(border + ring_thickness + border));
    graphics_context_set_fill_color(ctx, map_color(GColorBlack));
    graphics_fill_rect(ctx, center_rect, corner_radius, GCornersAll);

    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Calculate angles (0 = 12 o'clock, clockwise)
    int32_t minute_angle = (TRIG_MAX_ANGLE * t->tm_min / 60) - (TRIG_MAX_ANGLE / 4);
    int32_t hour_angle = (TRIG_MAX_ANGLE * (t->tm_hour % 12) / 12) + (TRIG_MAX_ANGLE * t->tm_min / 720) - (TRIG_MAX_ANGLE / 4);

    // For rectangular mode compute radial distances to the inset rectangles along the angle
    int16_t outer_inset = border;
    int16_t inner_inset = border + ring_thickness;

    int16_t minute_inner = radial_distance_to_inset(bounds, inner_inset, minute_angle) - 2;
    int16_t minute_outer = radial_distance_to_inset(bounds, outer_inset, minute_angle) + 3;

    int16_t hour_inner = radial_distance_to_inset(bounds, inner_inset, hour_angle) - 2;
    int16_t hour_outer = radial_distance_to_inset(bounds, outer_inset, hour_angle) + 3;

    // Clamp to sensible values
    if (minute_inner < 0) minute_inner = 0;
    if (hour_inner < 0) hour_inner = 0;

    // Draw markers (mapped colors)
    draw_marker(ctx, center, minute_angle, minute_inner, minute_outer, 10, map_color(PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)));
    draw_marker_with_border(ctx, center, hour_angle, hour_inner, hour_outer, 12, map_color(PBL_IF_COLOR_ELSE(s_hand_color, GColorDarkGray)), map_color(GColorLightGray), 2);

    // Draw center square
    int16_t w_size = bounds.size.w - 2*(border + ring_thickness + border);
    int16_t h_size = bounds.size.h - 2*(border + ring_thickness + border);
    GRect center_square = GRect(center.x - w_size/2, center.y - h_size/2, w_size, h_size);
    uint16_t center_corner_radius = 8;
    graphics_context_set_fill_color(ctx, map_color(GColorBlack));
    graphics_fill_rect(ctx, center_square, center_corner_radius, GCornersAll);

  } else {
    // Circular (default) behavior
    // Calculate radii for each layer
    int16_t outer_radius = (center.x < center.y ? center.x : center.y) - 1;
    int16_t r_outer_border = outer_radius;
    int16_t r_white_outer = outer_radius - border;
    int16_t r_white_inner = r_white_outer - ring_thickness;
    int16_t r_inner_border = r_white_inner;
    int16_t r_center = r_white_inner - border;

    // Draw background
    graphics_context_set_fill_color(ctx, map_color(GColorBlack));
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Draw outer dark gray border
    graphics_context_set_fill_color(ctx, map_color(GColorDarkGray));
    graphics_fill_circle(ctx, center, r_outer_border);

    // Draw white ring
    graphics_context_set_fill_color(ctx, map_color(GColorWhite));
    graphics_fill_circle(ctx, center, r_white_outer);

    // Draw inner dark gray border
    graphics_context_set_fill_color(ctx, map_color(GColorDarkGray));
    graphics_fill_circle(ctx, center, r_inner_border);

    // Draw black center
    graphics_context_set_fill_color(ctx, map_color(GColorBlack));
    graphics_fill_circle(ctx, center, r_center);

    // Get current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Calculate angles (0 = 12 o'clock, clockwise)
    int32_t minute_angle = (TRIG_MAX_ANGLE * t->tm_min / 60) - (TRIG_MAX_ANGLE / 4);
    int32_t hour_angle = (TRIG_MAX_ANGLE * (t->tm_hour % 12) / 12) + (TRIG_MAX_ANGLE * t->tm_min / 720) - (TRIG_MAX_ANGLE / 4);

    // Markers span only across the white ring (from inner edge to outer edge of white ring)
    int16_t marker_inner = r_white_inner - 2;  // slightly into inner border
    int16_t marker_outer = r_white_outer + 3;  // slightly into outer border

    // Draw minute marker first (behind hour)
    draw_marker(ctx, center, minute_angle, marker_inner, marker_outer, 10, map_color(PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)));

    // Draw hour marker on top - white with light gray border
    draw_marker_with_border(ctx, center, hour_angle, marker_inner, marker_outer, 12, map_color(PBL_IF_COLOR_ELSE(s_hand_color, GColorDarkGray)), map_color(GColorLightGray), 2);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

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

  s_main_window = window_create();
  window_set_background_color(s_main_window, map_color(GColorBlack));
  window_set_window_handlers(s_main_window, (WindowHandlers){
      .load = main_window_load,
      .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register AppMessage handler for settings
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(128, 64);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
