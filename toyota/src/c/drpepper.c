#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_steps_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static char s_time_buffer[8];
static char s_date_buffer[16];
static char s_steps_buffer[16];

static void update_steps() {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  if(mask & HealthServiceAccessibilityMaskAvailable) {
    snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d steps", (int)health_service_sum_today(metric));
    text_layer_set_text(s_steps_layer, s_steps_buffer);
  } else {
    text_layer_set_text(s_steps_layer, "N/A");
  }
}

static void health_handler(HealthEventType event, void *context) {
  if(event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    update_steps();
  }
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  if(clock_is_24h_style()) {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M", tick_time);
  } else {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%I:%M", tick_time);
  }
  text_layer_set_text(s_time_layer, s_time_buffer);

  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Background logo
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DR_PEPPER_LOGO);
  s_background_layer = bitmap_layer_create(bounds);
  if(s_background_bitmap) {
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  } else {
    window_set_background_color(window, GColorBlack);
  }
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  // Steps at bottom (0–24 from bottom → Y = 144, height = 24)
  s_steps_layer = text_layer_create(GRect(0, 144, bounds.size.w, 24));
  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_text_color(s_steps_layer, GColorWhite);
  text_layer_set_font(s_steps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));

  // Time at top (110–168 from bottom → Y = 0, height = 40)
  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, 40));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Date just below time, height increased to 28 to fit full font
  s_date_layer = text_layer_create(GRect(0, 40, bounds.size.w, 28));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  update_time();
  update_steps();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_steps_layer);
  if(s_background_bitmap) gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  health_service_events_subscribe(health_handler, NULL);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

