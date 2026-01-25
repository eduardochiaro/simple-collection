#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static GDrawCommandImage *s_number_6_white;
static GDrawCommandImage *s_number_6_black;
static GDrawCommandImage *s_number_2_white;
static GDrawCommandImage *s_number_2_black;
static GDrawCommandImage *s_number_10_white;
static GDrawCommandImage *s_number_10_black;

// Time tracking
static struct tm s_last_time;

// Settings
static bool s_invert_colors = false;

// Color helper functions
static GColor get_background_color() {
  return s_invert_colors ? GColorBlack : GColorWhite;
}

static GColor get_line_color() {
  return s_invert_colors ? PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite) : PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack);
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
  
  // Draw radial lines only for 10, 2, and 6 o'clock
  graphics_context_set_stroke_color(ctx, get_line_color());
  graphics_context_set_stroke_width(ctx, 1);
  
  // Use a large enough radius to ensure all lines reach edges
  int radius = bounds.size.w > bounds.size.h ? bounds.size.w : bounds.size.h;
  
  // 10 o'clock (position 10)
  int32_t angle_10 = TRIG_MAX_ANGLE * 10 / 12;
  GPoint outer_10 = {
    .x = (int16_t)(sin_lookup(angle_10) * radius / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle_10) * radius / TRIG_MAX_RATIO) + center.y,
  };
  graphics_draw_line(ctx, center, outer_10);
  
  // 2 o'clock (position 2)
  int32_t angle_2 = TRIG_MAX_ANGLE * 2 / 12;
  GPoint outer_2 = {
    .x = (int16_t)(sin_lookup(angle_2) * radius / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle_2) * radius / TRIG_MAX_RATIO) + center.y,
  };
  graphics_draw_line(ctx, center, outer_2);
  
  // 6 o'clock (position 6)
  int32_t angle_6 = TRIG_MAX_ANGLE * 6 / 12;
  GPoint outer_6 = {
    .x = (int16_t)(sin_lookup(angle_6) * radius / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle_6) * radius / TRIG_MAX_RATIO) + center.y,
  };
  graphics_draw_line(ctx, center, outer_6);
  
  // Draw black dots for other hour positions (3px size, 6px from screen border)
  graphics_context_set_fill_color(ctx, get_accent_color());
  
  // Calculate dot positions for rectangular and round screens
  #ifdef PBL_ROUND
    // For round screens (Chalk), position dots using radius calculation
    int dot_radius = (bounds.size.w / 2) - 6 - 2;  // 6px from border, 2px for dot radius
  #else
    // For rectangular screens, calculate distance to nearest edge at each angle
    int dot_radius = 0;  // Will be calculated per position
  #endif
  
  for (int i = 0; i < 12; i++) {
    // Skip 10, 2, and 6 o'clock (keep the lines)
    if (i == 10 || i == 2 || i == 6) continue;
    
    int32_t angle = TRIG_MAX_ANGLE * i / 12;
    
    #ifdef PBL_ROUND
      // Round screen: simple radius-based positioning
      GPoint dot_pos = {
        .x = (int16_t)(sin_lookup(angle) * dot_radius / TRIG_MAX_RATIO) + center.x,
        .y = (int16_t)(-cos_lookup(angle) * dot_radius / TRIG_MAX_RATIO) + center.y,
      };
    #else
      // Rectangular screen: calculate distance to nearest edge
      float sin_a = sin_lookup(angle) / (float)TRIG_MAX_RATIO;
      float cos_a = -cos_lookup(angle) / (float)TRIG_MAX_RATIO;
      
      // Calculate which edge we'll hit first
      float dist_to_edge;
      if (sin_a > 0.01) {
        // Moving right
        float dist_right = (bounds.size.w - center.x - 6) / sin_a;
        if (cos_a > 0.01 || cos_a < -0.01) {
          float dist_vert = (cos_a > 0) ? (bounds.size.h - center.y - 6) / cos_a : (center.y - 6) / -cos_a;
          dist_to_edge = (dist_right < dist_vert) ? dist_right : dist_vert;
        } else {
          dist_to_edge = dist_right;  // Purely horizontal (3 o'clock)
        }
      } else if (sin_a < -0.01) {
        // Moving left
        float dist_left = (center.x - 6) / -sin_a;
        if (cos_a > 0.01 || cos_a < -0.01) {
          float dist_vert = (cos_a > 0) ? (bounds.size.h - center.y - 6) / cos_a : (center.y - 6) / -cos_a;
          dist_to_edge = (dist_left < dist_vert) ? dist_left : dist_vert;
        } else {
          dist_to_edge = dist_left;  // Purely horizontal (9 o'clock)
        }
      } else {
        // sin_a == 0, moving purely vertical
        dist_to_edge = (cos_a > 0) ? (bounds.size.h - center.y - 6) : (center.y - 6);
      }
      
      GPoint dot_pos = {
        .x = (int16_t)(sin_a * dist_to_edge) + center.x,
        .y = (int16_t)(cos_a * dist_to_edge) + center.y,
      };
    #endif
    
    graphics_fill_circle(ctx, dot_pos, 1);  // 2px diameter = 1px radius
  }

  // Draw PDC number 10 at 10 o'clock position (12px from screen border)
  if (s_number_10_black && s_number_10_white) {
    GSize img_size_10 = gdraw_command_image_get_bounds_size(s_invert_colors ? s_number_10_white : s_number_10_black);
    
    // Calculate position along 10 o'clock line (300 degrees)
    int32_t angle_10_pos = TRIG_MAX_ANGLE * 10 / 12;
    float sin_a = sin_lookup(angle_10_pos) / (float)TRIG_MAX_RATIO;
    float cos_a = -cos_lookup(angle_10_pos) / (float)TRIG_MAX_RATIO;
    
    GPoint pos_10;
    #ifdef PBL_ROUND
      // Round screen: simple radius-based positioning
      int dist_10 = (bounds.size.w / 2) - 12 - (img_size_10.w / 2);
      pos_10.x = (int16_t)(sin_a * dist_10) + center.x - img_size_10.w / 2;
      pos_10.y = (int16_t)(cos_a * dist_10) + center.y - img_size_10.h / 2;
    #else
      // Rectangular screen: position along the line, accounting for which edge we hit
      // For 10 o'clock: moving up-left, will hit left edge or top edge
      // sin_a is negative (moving left), cos_a is negative (moving up)
      float dist_to_left = (6 + img_size_10.w / 2 - center.x) / sin_a;  // Negative distance
      float dist_to_top = (6 + img_size_10.h / 2 - center.y) / cos_a;   // Negative distance
      float dist = (dist_to_left < dist_to_top) ? dist_to_left : dist_to_top;  // Use less negative (closer)
      
      pos_10.x = (int16_t)(sin_a * dist) + center.x - img_size_10.w / 2;
      pos_10.y = (int16_t)(cos_a * dist) + center.y - img_size_10.h / 2;
    #endif
    
    // Draw background for number 10
    graphics_context_set_fill_color(ctx, get_background_color());
    graphics_fill_rect(ctx, GRect(pos_10.x + 2, pos_10.y + 2, img_size_10.w - 4, img_size_10.h - 4), 2, GCornersAll);
    
    // Draw number 10
    gdraw_command_image_draw(ctx, s_invert_colors ? s_number_10_white : s_number_10_black, pos_10);
  }

  // Draw PDC number 2 at 2 o'clock position (12px from screen border)
  if (s_number_2_black && s_number_2_white) {
    GSize img_size_2 = gdraw_command_image_get_bounds_size(s_invert_colors ? s_number_2_white : s_number_2_black);
    
    // Calculate position along 2 o'clock line (60 degrees)
    int32_t angle_2_pos = TRIG_MAX_ANGLE * 2 / 12;
    float sin_a = sin_lookup(angle_2_pos) / (float)TRIG_MAX_RATIO;
    float cos_a = -cos_lookup(angle_2_pos) / (float)TRIG_MAX_RATIO;
    
    GPoint pos_2;
    #ifdef PBL_ROUND
      // Round screen: simple radius-based positioning
      int dist_2 = (bounds.size.w / 2) - 12 - (img_size_2.w / 2);
      pos_2.x = (int16_t)(sin_a * dist_2) + center.x - img_size_2.w / 2;
      pos_2.y = (int16_t)(cos_a * dist_2) + center.y - img_size_2.h / 2;
    #else
      // Rectangular screen: position along the line, accounting for which edge we hit
      // For 2 o'clock: moving up-right, will hit right edge or top edge
      float dist_to_right = (bounds.size.w - 6 - img_size_2.w / 2 - center.x) / sin_a;
      float dist_to_top = (6 + img_size_2.h / 2 - center.y) / cos_a;
      float dist = (dist_to_right < dist_to_top) ? dist_to_right : dist_to_top;  // Use the smaller distance
      
      pos_2.x = (int16_t)(sin_a * dist) + center.x - img_size_2.w / 2;
      pos_2.y = (int16_t)(cos_a * dist) + center.y - img_size_2.h / 2;
    #endif
    
    // Draw background for number 2
    graphics_context_set_fill_color(ctx, get_background_color());
    graphics_fill_rect(ctx, GRect(pos_2.x + 2, pos_2.y + 2, img_size_2.w - 4, img_size_2.h - 4), 2, GCornersAll);
    
    // Draw number 2
    gdraw_command_image_draw(ctx, s_invert_colors ? s_number_2_white : s_number_2_black, pos_2);
  }

  // Draw PDC number 6 at bottom (12px from screen border)
  if (s_number_6_black && s_number_6_white) {
    GSize img_size = gdraw_command_image_get_bounds_size(s_invert_colors ? s_number_6_white : s_number_6_black);
    
    // Position 12px from bottom border
    int y_position = bounds.size.h - img_size.h - 12;

    // Draw background for number 6
    graphics_context_set_fill_color(ctx, get_background_color());
    graphics_fill_rect(ctx, GRect(center.x - (img_size.w - 4) / 2, y_position - 4, img_size.w - 4, img_size.h + 8), 2, GCornersAll);
    
    GRect img_rect = GRect(center.x - img_size.w / 2, y_position, img_size.w, img_size.h);
    gdraw_command_image_draw(ctx, s_invert_colors ? s_number_6_white : s_number_6_black, img_rect.origin);
  }
  
  // Calculate time values
  int hour = s_last_time.tm_hour % 12;
  int minute = s_last_time.tm_min;
  
  // Calculate hand angles
  int32_t minute_angle = TRIG_MAX_ANGLE * minute / 60;
  int32_t hour_angle = (TRIG_MAX_ANGLE * ((hour * 60) + minute)) / (12 * 60);
  
  // Draw hour hand (shorter, thicker, red)
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, get_hand_hour_color());
  GPoint hour_hand = {
    .x = (int16_t)(sin_lookup(hour_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(28, 44)) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(28, 44)) / TRIG_MAX_RATIO) + center.y,
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
  s_number_6_white = gdraw_command_image_create_with_resource(RESOURCE_ID_NUMBER_6_WHITE);
  s_number_6_black = gdraw_command_image_create_with_resource(RESOURCE_ID_NUMBER_6_BLACK);
  
  // Load NUMBER_2 PDC resource
  s_number_2_white = gdraw_command_image_create_with_resource(RESOURCE_ID_NUMBER_2_WHITE);
  s_number_2_black = gdraw_command_image_create_with_resource(RESOURCE_ID_NUMBER_2_BLACK);
  
  // Load NUMBER_10 PDC resource
  s_number_10_white = gdraw_command_image_create_with_resource(RESOURCE_ID_NUMBER_10_WHITE);
  s_number_10_black = gdraw_command_image_create_with_resource(RESOURCE_ID_NUMBER_10_BLACK);
  
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
  gdraw_command_image_destroy(s_number_6_white);
  gdraw_command_image_destroy(s_number_6_black);
  gdraw_command_image_destroy(s_number_2_white);
  gdraw_command_image_destroy(s_number_2_black);
  gdraw_command_image_destroy(s_number_10_white);
  gdraw_command_image_destroy(s_number_10_black);
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
