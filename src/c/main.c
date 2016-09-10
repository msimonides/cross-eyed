#include <pebble.h>
#include "timeface.h"

static Window* s_main_window;
static Layer* s_watchface_layer;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  timeface_layer_set_time(s_watchface_layer, tick_time);
}

static void main_window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  s_watchface_layer = timeface_layer_create(window_bounds);
  
  time_t now = time(NULL);
  timeface_layer_set_time(s_watchface_layer, localtime(&now));
  
  layer_add_child(window_layer, s_watchface_layer);
  
  window_set_background_color(window, GColorWhite);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void main_window_unload(Window* window) {
  layer_destroy(s_watchface_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}