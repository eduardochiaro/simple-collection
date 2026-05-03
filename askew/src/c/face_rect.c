#include "face_rect.h"
#include "face_common.h"

// Corner radius for the rounded rectangle
#define RECT_CORNER_R 12

void face_rect_draw(GContext *ctx, GRect bounds, struct tm *t) {
  GPoint center = grect_center_point(&bounds);
  int inset  = 10;
  int hw     = bounds.size.w / 2 - inset;  // half-width of rounded rect
  int hh     = bounds.size.h / 2 - inset;  // half-height of rounded rect
  int radius = bounds.size.w > bounds.size.h ? bounds.size.w : bounds.size.h;

  // Background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // --- Radial lines FIRST (behind the rect) ---

  // Minute ticks
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorWindsorTan, GColorWhite));
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 0; i < 60; i++) {
    if (i % 5 == 0) continue;
    int32_t a = TRIG_MAX_ANGLE * i / 60 + ROTATION_OFFSET;
    GPoint inner = {
      .x = (int16_t)(sin_lookup(a) * (hw + 2) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(a) * (hw + 2) / TRIG_MAX_RATIO) + center.y,
    };
    GPoint outer = {
      .x = (int16_t)(sin_lookup(a) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(a) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, inner, outer);
  }

  // Hour lines
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed, GColorWhite));
  graphics_context_set_stroke_width(ctx, 3);
  for (int i = 0; i < 12; i++) {
    int32_t a = TRIG_MAX_ANGLE * i / 12 + ROTATION_OFFSET;
    GPoint inner = {
      .x = (int16_t)(sin_lookup(a) * (hw + 2) / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(a) * (hw + 2) / TRIG_MAX_RATIO) + center.y,
    };
    GPoint outer = {
      .x = (int16_t)(sin_lookup(a) * radius / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(a) * radius / TRIG_MAX_RATIO) + center.y,
    };
    graphics_draw_line(ctx, inner, outer);
  }

  // --- Rounded rectangle (12-point GPath polygon approximation) ---
  // Each corner arc is approximated with a 45° midpoint, giving convex rounded corners.
  // c707 = r*sin(45°), c293 = r*(1-sin(45°)) — used for the arc bulge point.
  int c = RECT_CORNER_R;
  int c707 = c * 46341 / 65536;
  int c293 = c - c707;
  GPoint rrect_pts[12] = {
    {  hw - c,    -hh       },   // TR start
    {  hw - c293, -hh + c293},   // TR arc mid
    {  hw,        -hh + c   },   // TR end
    {  hw,         hh - c   },   // BR start
    {  hw - c293,  hh - c293},   // BR arc mid
    {  hw - c,     hh       },   // BR end
    { -hw + c,     hh       },   // BL start
    { -hw + c293,  hh - c293},   // BL arc mid
    { -hw,         hh - c   },   // BL end
    { -hw,        -hh + c   },   // TL start
    { -hw + c293, -hh + c293},   // TL arc mid
    { -hw + c,    -hh       },   // TL end
  };
  GPathInfo rrect_info = { .num_points = 12, .points = rrect_pts };
  GPath *rrect = gpath_create(&rrect_info);
  gpath_rotate_to(rrect, ROTATION_OFFSET);
  gpath_move_to(rrect, center);
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, rrect);
  gpath_destroy(rrect);

  // --- Band, clipped to rectangle width ---
  // Use hw as the band half-width so it stops at the rect edges
  int band_half = bounds.size.w >= 200 ? 40 : 25;
  GPoint band_pts[4] = {
    { -hw, -band_half },
    {  hw, -band_half },
    {  hw,  band_half },
    { -hw,  band_half },
  };
  GPathInfo band_info = { .num_points = 4, .points = band_pts };
  GPath *band = gpath_create(&band_info);
  gpath_rotate_to(band, ROTATION_OFFSET);
  gpath_move_to(band, center);
  graphics_context_set_fill_color(ctx, GColorLightGray);
  gpath_draw_filled(ctx, band);
  gpath_destroy(band);

  draw_hands(ctx, bounds, t);
}
