#include "face_round.h"
#include "face_common.h"

void face_round_draw(GContext *ctx, GRect bounds, struct tm *t) {
  GPoint center = grect_center_point(&bounds);
  int circle_r = bounds.size.w / 2 - 16;
  int radius   = bounds.size.w > bounds.size.h ? bounds.size.w : bounds.size.h;

  // Background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // White filled circle
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, circle_r);

  // Band
  int band_half = bounds.size.w >= 200 ? 40 : 25;
  GPoint band_pts[4] = {
    { -bounds.size.w, -band_half },
    {  bounds.size.w, -band_half },
    {  bounds.size.w,  band_half },
    { -bounds.size.w,  band_half },
  };
  GPathInfo band_info = { .num_points = 4, .points = band_pts };
  GPath *band = gpath_create(&band_info);
  gpath_rotate_to(band, ROTATION_OFFSET);
  gpath_move_to(band, center);
  graphics_context_set_fill_color(ctx, GColorLightGray);
  gpath_draw_filled(ctx, band);
  gpath_destroy(band);

  // Mask overflow with a thick black arc
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 30);
  int arc_r = circle_r + 15;
  GRect arc_rect = GRect(center.x - arc_r, center.y - arc_r, arc_r * 2, arc_r * 2);
  graphics_draw_arc(ctx, arc_rect, GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE);

  // Radial minute ticks (behind radial hour lines, from circle edge outward)
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorWhite));
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 0; i < 60; i++) {
    if (i % 5 == 0) continue;
    int32_t a = TRIG_MAX_ANGLE * i / 60 + ROTATION_OFFSET;
    GPoint inner = {
      .x = (int16_t)(sin_lookup(a) * (circle_r + 2) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(a) * (circle_r + 2) / TRIG_MAX_RATIO) + center.y,
    };
    GPoint outer = {
      .x = (int16_t)(sin_lookup(a) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(a) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, inner, outer);
  }

  // Radial hour lines
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed, GColorWhite));
  graphics_context_set_stroke_width(ctx, 3);
  for (int i = 0; i < 12; i++) {
    int32_t a = TRIG_MAX_ANGLE * i / 12 + ROTATION_OFFSET;
    GPoint inner = {
      .x = (int16_t)(sin_lookup(a) * (circle_r + 2) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(a) * (circle_r + 2) / TRIG_MAX_RATIO) + center.y,
    };
    GPoint outer = {
      .x = (int16_t)(sin_lookup(a) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(a) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, inner, outer);
  }

  draw_hands(ctx, bounds, t);
}
