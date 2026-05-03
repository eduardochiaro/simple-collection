#pragma once
#include <pebble.h>

#define ANGLE 20
#define ROTATION_OFFSET (TRIG_MAX_ANGLE * ANGLE / 360)

#if ANGLE == 10
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_10
  #define TEXT_PEBBLE_LARGE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_10_LARGE
#elif ANGLE == 15
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_15
  #define TEXT_PEBBLE_LARGE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_15_LARGE
#elif ANGLE == 20
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_20
  #define TEXT_PEBBLE_LARGE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_20_LARGE
#elif ANGLE == 25
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_25
  #define TEXT_PEBBLE_LARGE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_25_LARGE
#elif ANGLE == 30
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_30
  #define TEXT_PEBBLE_LARGE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_30_LARGE
#else
  #define TEXT_PEBBLE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_0
  #define TEXT_PEBBLE_LARGE_RESOURCE RESOURCE_ID_TEXT_PEBBLE_0_LARGE
#endif

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

  int32_t minute_angle = TRIG_MAX_ANGLE * minute / 60 + ROTATION_OFFSET;
  int32_t hour_angle   = (TRIG_MAX_ANGLE * ((hour * 60) + minute)) / (12 * 60) + ROTATION_OFFSET;

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
