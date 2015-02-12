#include "pebble.h"
#include "QTPlus.h"

GColor background_color = GColorBlack;

static Window *window;
static Layer *window_layer;

static AppSync sync;
static uint8_t sync_buffer[128];

static int blink;
static int invert;
static int bluetoothvibe;
static int hourlyvibe;
static int secs;
static int timeformat;

static bool appStarted = false;

enum {
//  BLINK_KEY = 0x0,
  INVERT_COLOR_KEY = 0x1,
  BLUETOOTHVIBE_KEY = 0x2,
  HOURLYVIBE_KEY = 0x3,
  SECS_KEY = 0x4,
  TIMEFORMAT_KEY = 0x5
};

static GBitmap *separator_image;
static BitmapLayer *separator_layer;

static GBitmap *time_format_image;
static BitmapLayer *time_format_layer;

static GBitmap *bar_image;
static BitmapLayer *bar_layer;

InverterLayer *inverter_layer = NULL;

#define TOTAL_TIME_DIGITS 2
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];


const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

#define TOTAL_MIN_DIGITS 2
static GBitmap *min_digits_images[TOTAL_MIN_DIGITS];
static BitmapLayer *min_digits_layers[TOTAL_MIN_DIGITS];


const int MIN_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_MIN_0,
  RESOURCE_ID_IMAGE_MIN_1,
  RESOURCE_ID_IMAGE_MIN_2,
  RESOURCE_ID_IMAGE_MIN_3,
  RESOURCE_ID_IMAGE_MIN_4,
  RESOURCE_ID_IMAGE_MIN_5,
  RESOURCE_ID_IMAGE_MIN_6,
  RESOURCE_ID_IMAGE_MIN_7,
  RESOURCE_ID_IMAGE_MIN_8,
  RESOURCE_ID_IMAGE_MIN_9
};

const int TINY_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_TINY_0,
  RESOURCE_ID_IMAGE_TINY_1,
  RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3,
  RESOURCE_ID_IMAGE_TINY_4,
  RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6,
  RESOURCE_ID_IMAGE_TINY_7,
  RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9,

};

#define TOTAL_SECONDS_DIGITS 2
static GBitmap *seconds_digits_images[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers[TOTAL_SECONDS_DIGITS];


void change_background(bool invert) {

  if (invert && inverter_layer == NULL) {
    // Add inverter layer
    Layer *window_layer = window_get_root_layer(window);

    inverter_layer = inverter_layer_create(GRect(70, 0, 52, 168));
    layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
  } else if (!invert && inverter_layer != NULL) {
    // Remove Inverter layer
    layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
    inverter_layer_destroy(inverter_layer);
    inverter_layer = NULL;
  }
	// No action required
}


static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
/*    case BLINK_KEY:
      blink = new_tuple->value->uint8 !=0;
	  persist_write_bool(BLINK_KEY, blink);

      tick_timer_service_unsubscribe();
      if(blink) {
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
      else {
		        layer_set_hidden(bitmap_layer_get_layer(separator_layer), true);

		tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

      }
      break;
	*/  
	case INVERT_COLOR_KEY:
      invert = new_tuple->value->uint8 != 0;
	  persist_write_bool(INVERT_COLOR_KEY, invert);
      change_background(invert);
      break;
	
    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break;      
	  
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
      break;
	  
	case SECS_KEY:
      secs = new_tuple->value->uint8 !=0;
	  	  persist_write_bool(SECS_KEY, secs);	  

	  if (appStarted & !secs) { 
		  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
	      layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[0]), true);
	      layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[1]), true);
		  
	  } else {
	    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
 		layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[0]), false);
	    layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[1]), false);

	  }
	  
      break;
	  
	  case TIMEFORMAT_KEY:
      timeformat = new_tuple->value->uint8 !=0;
	  	  persist_write_bool(TIMEFORMAT_KEY, timeformat);	  

	  if (timeformat) { 
	      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), true);
	  } else {
	      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), false);
	  }
      break;
  
  }
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;
  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
  gbitmap_destroy(old_image);
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void toggle_bluetooth(bool connected) {
  if(appStarted && !connected && bluetoothvibe) {
	
 //vibe!
    vibes_long_pulse();
  }
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth(connected);
}

static void update_hours(struct tm *tick_time) {

  if(appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
  }
  
   unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(11, 37));
  set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(39, 37));
	

  if (!clock_is_24h_style()) {
    if (tick_time->tm_hour >= 12) {
      set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_MODE_PM, GPoint(70, 108));
      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), false);
    } 
    else {
       set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_MODE_AM, GPoint(70, 108));
    }
    
    if (display_hour/10 == 0) {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
    }
    else {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
    }
  }
}

static void update_minutes(struct tm *tick_time) {
  set_container_image(&min_digits_images[0], min_digits_layers[0], MIN_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(70, 37));
  set_container_image(&min_digits_images[1], min_digits_layers[1], MIN_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(96, 37));
	
}

static void update_seconds(struct tm *tick_time) {
  set_container_image(&seconds_digits_images[0], seconds_digits_layers[0], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_sec/10], GPoint(123, 37));
  set_container_image(&seconds_digits_images[1], seconds_digits_layers[1], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_sec%10], GPoint(123, 66));

  if(blink) {
    layer_set_hidden(bitmap_layer_get_layer(separator_layer), tick_time->tm_sec%2);
  }
  else {
    if(layer_get_hidden(bitmap_layer_get_layer(separator_layer))) {
      layer_set_hidden(bitmap_layer_get_layer(separator_layer), false);
    }
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {

  if (units_changed & HOUR_UNIT) {
    update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
    update_minutes(tick_time);
  }	
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }		
}

void set_style(void) {
    background_color  = GColorBlack;
    window_set_background_color(window, background_color);
}

static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));
	
  memset(&min_digits_layers, 0, sizeof(min_digits_layers));
  memset(&min_digits_images, 0, sizeof(min_digits_images));

  memset(&seconds_digits_layers, 0, sizeof(seconds_digits_layers));
  memset(&seconds_digits_images, 0, sizeof(seconds_digits_images));

  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);  

  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }
	
  set_style();
	
  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);

  bar_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAR);
  GRect frame2 = (GRect) {
    .origin = { .x = 63, .y = 37 },
    .size = bar_image->bounds.size
  };
  bar_layer = bitmap_layer_create(frame2);
  bitmap_layer_set_bitmap(bar_layer, bar_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bar_layer));  

  separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEP);
  GRect frame = (GRect) {
    .origin = { .x = 63, .y = 37 },
    .size = separator_image->bounds.size
  };
  separator_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(separator_layer, separator_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(separator_layer));   

  GRect frame5 = (GRect) {
    .origin = { .x = 70, .y = 108 },
    .size = {.w = 52, .h = 23}
  };
  time_format_layer = bitmap_layer_create(frame5);
  if (clock_is_24h_style()) {
    time_format_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MODE_24H);
    bitmap_layer_set_bitmap(time_format_layer, time_format_image);
    
  }
  layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));	
	
	
  // Create time and date layers
  GRect dummy_frame = { {0, 0}, {0, 0} };

  for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
	  layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
	
 for (int i = 0; i < TOTAL_MIN_DIGITS; ++i) {
    min_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(min_digits_layers[i]));
  }
	
for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers[i]));
  }
	
	  toggle_bluetooth(bluetooth_connection_service_peek());

	
   Tuplet initial_values[] = {
//    TupletInteger(BLINK_KEY, persist_read_bool(BLINK_KEY)),
    TupletInteger(INVERT_COLOR_KEY, persist_read_bool(INVERT_COLOR_KEY)),
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(SECS_KEY, persist_read_bool(SECS_KEY)),
    TupletInteger(TIMEFORMAT_KEY, persist_read_bool(TIMEFORMAT_KEY)),
  };
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, NULL, NULL);
   
  appStarted = true;
  
  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, MONTH_UNIT + DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);	
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);

}

static void deinit(void) {
  app_sync_deinit(&sync);

  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
	
  layer_remove_from_parent(bitmap_layer_get_layer(separator_layer));
  bitmap_layer_destroy(separator_layer);
  gbitmap_destroy(separator_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
  bitmap_layer_destroy(time_format_layer);
  gbitmap_destroy(time_format_image);

  layer_remove_from_parent(bitmap_layer_get_layer(bar_layer));
  bitmap_layer_destroy(bar_layer);
  gbitmap_destroy(bar_image);
  bar_image = NULL;


   for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    bitmap_layer_destroy(time_digits_layers[i]);
  }

	
   for (int i = 0; i < TOTAL_MIN_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(min_digits_layers[i]));
    gbitmap_destroy(min_digits_images[i]);
    bitmap_layer_destroy(min_digits_layers[i]);
  }
	
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(seconds_digits_layers[i]));
    gbitmap_destroy(seconds_digits_images[i]);
    bitmap_layer_destroy(seconds_digits_layers[i]);
  }	


   layer_remove_from_parent(window_layer);
   layer_destroy(window_layer);
	
   qtp_app_deinit();

	
}

int main(void) {
  init();
  qtp_setup();
  app_event_loop();
  deinit();
}