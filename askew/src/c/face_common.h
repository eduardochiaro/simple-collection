#pragma once
#include <pebble.h>

#define DEFAULT_ANGLE 20

// Runtime rotation offset — set from persisted settings in simple-askew.c
extern int32_t g_rotation_offset;

// Map angle value to the correct pre-rotated text bitmap resource ID
static inline uint32_t get_text_resource_id(int angle, bool large) {
  if (large) {
    switch (angle) {
      case 10: return RESOURCE_ID_TEXT_PEBBLE_10_LARGE;
      case 15: return RESOURCE_ID_TEXT_PEBBLE_15_LARGE;
      case 20: return RESOURCE_ID_TEXT_PEBBLE_20_LARGE;
      case 25: return RESOURCE_ID_TEXT_PEBBLE_25_LARGE;
      case 30: return RESOURCE_ID_TEXT_PEBBLE_30_LARGE;
      default: return RESOURCE_ID_TEXT_PEBBLE_0_LARGE;
    }
  } else {
    switch (angle) {
      case 10: return RESOURCE_ID_TEXT_PEBBLE_10;
      case 15: return RESOURCE_ID_TEXT_PEBBLE_15;
      case 20: return RESOURCE_ID_TEXT_PEBBLE_20;
      case 25: return RESOURCE_ID_TEXT_PEBBLE_25;
      case 30: return RESOURCE_ID_TEXT_PEBBLE_30;
      default: return RESOURCE_ID_TEXT_PEBBLE_0;
    }
  }
}

static inline GColor get_hand_hour_color() {
#ifdef PBL_COLOR
  return GColorChromeYellow;
#else
  return GColorBlack;
#endif
}

static inline GColor get_hand_minute_color() {
#ifdef PBL_COLOR
  return GColorRajah;
#else
  return GColorBlack;
#endif
}

// Draw the clock hands and center dot (shared by both faces)
static inline void draw_hands(GContext *ctx, GRect bounds, struct tm *t) {
  GPoint center = grect_center_point(&bounds);
  int hour   = t->tm_hour % 12;
  int minute = t->tm_min;

  int32_t minute_angle = TRIG_MAX_ANGLE * minute / 60 + g_rotation_offset;
  int32_t hour_angle   = (TRIG_MAX_ANGLE * ((hour * 60) + minute)) / (12 * 60) + g_rotation_offset;

  // Hour hand
  graphics_context_set_stroke_width(ctx, 4);
  graphics_context_set_stroke_color(ctx, get_hand_hour_color());
  int hour_hand_offset = bounds.size.w >= 201 ? 64 : PBL_IF_RECT_ELSE(36, 44);
  GPoint hour_tip = {
    .x = (int16_t)(sin_lookup(hour_angle) * (bounds.size.w / 2 - hour_hand_offset) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (bounds.size.w / 2 - hour_hand_offset) / TRIG_MAX_RATIO) + center.y,
  };
  GPoint hour_tail = {
    .x = (int16_t)(-sin_lookup(hour_angle) * 16 / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(cos_lookup(hour_angle)  * 16 / TRIG_MAX_RATIO) + center.y,
  };
  graphics_draw_line(ctx, hour_tail, center);
  graphics_draw_line(ctx, center, hour_tip);

  // Minute hand
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, get_hand_minute_color());
  GPoint min_tip = {
    .x = (int16_t)(sin_lookup(minute_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(20, 22)) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (bounds.size.w / 2 - PBL_IF_RECT_ELSE(20, 22)) / TRIG_MAX_RATIO) + center.y,
  };
  GPoint min_tail = {
    .x = (int16_t)(-sin_lookup(minute_angle) * 16 / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(cos_lookup(minute_angle)  * 16 / TRIG_MAX_RATIO) + center.y,
  };
  graphics_draw_line(ctx, min_tail, center);
  graphics_draw_line(ctx, center, min_tip);

  // Center dot
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_circle(ctx, center, 4);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 3);
}
