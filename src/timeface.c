#include <pebble.h>
#include "timeface.h"

#define MOUTH_CHANGE_MODULO_MINUTES 5

#define PI 3.14159265

#define ARRAYSIZE(x) sizeof((x))/sizeof((x)[0])

#define PATH_SCALE 100
#define EYE_POLYGON_POINTS 8
static GPoint SQUINT[] = { { -45, 10}, { -27, 20}, { 35, 20}, { 45, 10},
                           { 45, -10 }, { 27, -20 }, { -35, -20 }, { -45, -10 } };
static GPoint OPEN[] = { { -35, 22 }, { -17, 42 }, { 17, 42 }, { 35, 22 },
                         { 32, -22 }, { 17, -42 }, { -17, -42}, { -32, -22 } };

#define BROW_POLYGON_POINTS 4
static GPoint BROW_DOWN[] = { {-45, -43}, { -15, -43}, { 15, -43 }, { 45, -43} };
static GPoint BROW_UP[] = { {-35, -60}, { -15, -70}, { 15, -70 }, { 35, -60} };

static GPoint MOUTH_SMILE[] = { { -55, -10 }, { -25, -5 }, {25, -5}, { 55, -10 },
                                { 35, 2 }, { 20, 15 }, { -20, 15 }, { -35, 2 } };
static GPoint MOUTH_SERIOUS[] = { { -30, 5 }, { 30, 5 } };
static GPoint MOUTH_LEFT[] = { { -40, -5 }, { -10, 0 } };
static GPoint MOUTH_RIGHT[] = { { 40, -5 }, { 10, 0 } };
static GPoint MOUTH_O[] = { { -12, 6 }, { -6, 12 }, { 7, 12 }, { 12, 6 },
                            { 12, -6 }, { 6, -12 }, { -7, -12 }, { -12, -6 } };

static GPathInfo MOUTHS[] = { { ARRAYSIZE(MOUTH_SMILE), MOUTH_SMILE },
                              { ARRAYSIZE(MOUTH_SERIOUS), MOUTH_SERIOUS },
                              { ARRAYSIZE(MOUTH_LEFT), MOUTH_LEFT },
                              { ARRAYSIZE(MOUTH_RIGHT), MOUTH_RIGHT },
                              { ARRAYSIZE(MOUTH_O), MOUTH_O } };

typedef struct LayerData {
  struct tm current_time;
  int mouth_index;
} LayerData;

static GPoint add_points(GPoint a, GPoint b) {
  return GPoint(a.x + b.x, a.y + b.y);
}

static void tween_points(const GPoint* src, const GPoint* dst, GPoint* result, int num_points, int percent) {
  for (int i = 0; i < num_points; ++i) {
    result[i].x = src[i].x + (dst[i].x - src[i].x) * percent / 100;
    result[i].y = src[i].y + (dst[i].y - src[i].y) * percent / 100;
  }
}

static void scale_points(const GPoint* source, GPoint* result, int num_points, float scale) {
  for (int i = 0; i < num_points; ++i)
    result[i] = GPoint(source[i].x * scale + 0.5f, source[i].y * scale + 0.5f);
}

static void draw_line_of_points(GContext* ctx, GPoint* points, int num_points, GPoint offset, bool closed) {
  for (int i = 0; i < num_points - 1; ++i) {
    graphics_draw_line(ctx,
                       add_points(points[i], offset),
                       add_points(points[i + 1], offset));
  }
  if (closed) {
    graphics_draw_line(ctx,
                       add_points(points[num_points - 1], offset),
                       add_points(points[0], offset));
  }
}

static void choose_random_mouth(Layer* layer) {
  LayerData* data = (LayerData*) layer_get_data(layer);
  data->mouth_index = rand() % ARRAYSIZE(MOUTHS);
}

static void draw_eye(GContext* ctx, GPoint center, uint16_t size, uint16_t angle) {
  int eye_radius = size / 2; 
  int pupil_radius = size / 5;
  int length = eye_radius - pupil_radius;
  int x = sin_lookup(angle);
  int y = -cos_lookup(angle);
  GPoint pupil_center = GPoint(x * length / TRIG_MAX_RATIO, y * length / TRIG_MAX_RATIO);

  float eye_offset_x = size / 20;
  float eye_offset_y = size / 10;
  int tween_percent = abs(y * 100 / TRIG_MAX_RATIO);
  float path_scale = ((float) size) / PATH_SCALE;
  GPoint eye_center = add_points(center, GPoint(x * eye_offset_x / TRIG_MAX_RATIO, y * eye_offset_y / TRIG_MAX_RATIO));
  
  GPoint outline[EYE_POLYGON_POINTS];
  tween_points(SQUINT, OPEN, outline, EYE_POLYGON_POINTS, tween_percent);
  scale_points(outline, outline, EYE_POLYGON_POINTS, path_scale);
  draw_line_of_points(ctx, outline, EYE_POLYGON_POINTS, eye_center, true);

  GPoint brow[BROW_POLYGON_POINTS];
  tween_points(BROW_DOWN, BROW_UP, brow, BROW_POLYGON_POINTS, tween_percent);
  scale_points(brow, brow, BROW_POLYGON_POINTS, path_scale);
  draw_line_of_points(ctx, brow, BROW_POLYGON_POINTS, eye_center, false);
  
  pupil_center.x += center.x;
  pupil_center.y += center.y;
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, pupil_center, pupil_radius);
}

static void draw_mouth(GContext* ctx, int mouth_index, GPoint center, uint16_t size) {
  GPathInfo path = MOUTHS[mouth_index];
  float path_scale = ((float) size) / PATH_SCALE;
    
  GPoint mouth[path.num_points];
  scale_points(path.points, mouth, path.num_points, path_scale);
  draw_line_of_points(ctx, mouth, path.num_points, center, true);
}

static void update_proc(Layer* layer, GContext* ctx) {
  LayerData* data = (LayerData*) layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 5);
  
  int eye_size = bounds.size.w * 0.33;
  int eye_center_y = bounds.size.h * PBL_IF_RECT_ELSE(0.4, 0.45);
  int left_eye_center_x = (bounds.size.w - 2 * eye_size) / 3 + eye_size / 2;
  int right_eye_center_x = bounds.size.w - ((bounds.size.w - 2 * eye_size) / 3 + eye_size / 2);
  
  int minutes = data->current_time.tm_min;
  int hour12 = (data->current_time.tm_hour) % 12;
  int hour12_minutes = hour12 * 60 + minutes;
  draw_eye(ctx, GPoint(left_eye_center_x, eye_center_y), eye_size, hour12_minutes * TRIG_MAX_ANGLE / (12 * 60));
  draw_eye(ctx, GPoint(right_eye_center_x, eye_center_y), eye_size, minutes * TRIG_MAX_ANGLE / 60);
  
  if (minutes % MOUTH_CHANGE_MODULO_MINUTES == 0)
    choose_random_mouth(layer);
  
  GPoint mouth_center = GPoint(bounds.size.w / 2, bounds.size.h * 0.75f);
  draw_mouth(ctx, data->mouth_index, mouth_center, bounds.size.w / PBL_IF_RECT_ELSE(2, 2.5f));
}

Layer* timeface_layer_create(GRect frame) {
  Layer* layer = layer_create_with_data(frame, sizeof(LayerData));
  layer_set_update_proc(layer, &update_proc);
  choose_random_mouth(layer);
  return layer;
}

void timeface_layer_set_time(Layer* layer, struct tm *tick_time) {
  LayerData* data = (LayerData*) layer_get_data(layer);
  data->current_time = *tick_time;
  layer_mark_dirty(layer);
}