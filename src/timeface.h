#pragma once
#include <pebble.h>

Layer* timeface_layer_create(GRect frame);

void timeface_layer_set_time(Layer* layer, struct tm *tick_time);