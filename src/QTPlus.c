#include "QTPlus.h"

GColor qtp_background_color = GColorBlack;
static uint8_t batteryPercent;

static AppTimer *display_timer;
static TextLayer *qtp_date_layer;
int cur_day = -1;

static GBitmap *battery_image;
static BitmapLayer *battery_image_layer;
static BitmapLayer *battery_layer;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

#define TOTAL_BATTERY_PERCENT_DIGITS 3
static GBitmap *battery_percent_image[TOTAL_BATTERY_PERCENT_DIGITS];
static BitmapLayer *battery_percent_layers[TOTAL_BATTERY_PERCENT_DIGITS];

const int BATT_TINY_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_BATT_TINY_0,
  RESOURCE_ID_IMAGE_BATT_TINY_1,
  RESOURCE_ID_IMAGE_BATT_TINY_2,
  RESOURCE_ID_IMAGE_BATT_TINY_3,
  RESOURCE_ID_IMAGE_BATT_TINY_4,
  RESOURCE_ID_IMAGE_BATT_TINY_5,
  RESOURCE_ID_IMAGE_BATT_TINY_6,
  RESOURCE_ID_IMAGE_BATT_TINY_7,
  RESOURCE_ID_IMAGE_BATT_TINY_8,
  RESOURCE_ID_IMAGE_BATT_TINY_9,
  RESOURCE_ID_IMAGE_TINY_PERCENT
};

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};


/* Initialize listeners to show and hide Quick Tap Plus as well as update data */
void qtp_setup() {
	qtp_is_showing = false;
	accel_tap_service_subscribe(&qtp_tap_handler);
		
}

/* Handle taps from the hardware */
void qtp_tap_handler(AccelAxisType axis, int32_t direction) {
	if (qtp_is_showing) {
		qtp_hide();
	} else {
		qtp_show();
	}
	qtp_is_showing = !qtp_is_showing;
}

/* Subscribe to taps and pass them to the handler */
void qtp_click_config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_BACK, qtp_back_click_responder);
}

/* Unusued. Subscribe to back button to exit */
void qtp_back_click_responder(ClickRecognizerRef recognizer, void *context) {
	qtp_hide();
}

void change_battery_icon(bool charging) {
 // gbitmap_destroy(battery_image);
  if(charging) {
    battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGE);
  }
  else {
    battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  }  
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  layer_mark_dirty(bitmap_layer_get_layer(battery_image_layer));
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void set_container_image2(GBitmap **bmp_image2, BitmapLayer *bmp_layer2, const int resource_id2, GPoint origin) {
  GBitmap *old_image2 = *bmp_image2;
  *bmp_image2 = gbitmap_create_with_resource(resource_id2);
  GRect frame2 = (GRect) {
    .origin = origin,
    .size = (*bmp_image2)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer2, *bmp_image2);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer2), frame2);
  gbitmap_destroy(old_image2);
}

static void update_battery(BatteryChargeState charge_state) {

  batteryPercent = charge_state.charge_percent;

  if(batteryPercent==100) {
        change_battery_icon(false);
        layer_set_hidden(bitmap_layer_get_layer(battery_layer), false);
    for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
      layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), true);
    }  
    return;
  }

  layer_set_hidden(bitmap_layer_get_layer(battery_layer), charge_state.is_charging);
  change_battery_icon(charge_state.is_charging);

 for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), false);
  }  
  set_container_image2(&battery_percent_image[0], battery_percent_layers[0], BATT_TINY_IMAGE_RESOURCE_IDS[charge_state.charge_percent/10], GPoint(123, 140));
  set_container_image2(&battery_percent_image[1], battery_percent_layers[1], BATT_TINY_IMAGE_RESOURCE_IDS[charge_state.charge_percent%10], GPoint(131, 140));
  set_container_image2(&battery_percent_image[2], battery_percent_layers[2], BATT_TINY_IMAGE_RESOURCE_IDS[10], GPoint(138, 140));
 
}

void battery_layer_update_callback(Layer *me, GContext* ctx) {        
  //draw the remaining battery percentage
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(2, 2, ((batteryPercent/100.0)*13.0), 5), 0, GCornerNone);
}

void qtp_update_date(struct tm *tick_time) {

	static char date_text[] = "xxxxxxxxx 00";
		
	// Only update the date when it's changed.
    int new_cur_day = tick_time->tm_year*1000 + tick_time->tm_yday;
    if (new_cur_day != cur_day) {
        cur_day = new_cur_day;
	}
        strftime(date_text, sizeof(date_text), "%B %e", tick_time);
        text_layer_set_text(qtp_date_layer, date_text);

}

void force_update(void) {
    time_t now = time(NULL);
    qtp_update_date(localtime(&now));
}

static void update_days(struct tm *tick_time) {
  set_container_image2(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint(0, 0));
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }

}
/* Auto-hide the window after a certain time */
void qtp_timeout() {
	qtp_hide();
	qtp_is_showing = false;
}

/* Create the QTPlus Window and initialize the layres */
void qtp_init() {
	qtp_window = window_create();

	memset(&battery_percent_layers, 0, sizeof(battery_percent_layers));
    memset(&battery_percent_image, 0, sizeof(battery_percent_image));

	qtp_background_color  = GColorBlack;
    window_set_background_color(qtp_window, qtp_background_color);
  Layer *qtp_window_layer = window_get_root_layer(qtp_window);
	
GRect date_frame = GRect( 0,142,122,32 );
		qtp_date_layer = text_layer_create(date_frame);
	    text_layer_set_text_color(qtp_date_layer, GColorWhite);
        text_layer_set_background_color(qtp_date_layer, GColorClear);
		text_layer_set_text_alignment(qtp_date_layer, GTextAlignmentRight);
		//text_layer_set_font(qtp_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	    text_layer_set_font(qtp_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SKA_16)));
		layer_add_child(window_get_root_layer(qtp_window), text_layer_get_layer(qtp_date_layer));
	
  battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  GRect framebatt = (GRect) {
    .origin = { .x = 124, .y = 152 },
    .size = battery_image->bounds.size
  };
  battery_layer = bitmap_layer_create(framebatt);
  battery_image_layer = bitmap_layer_create(framebatt);
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  layer_set_update_proc(bitmap_layer_get_layer(battery_layer), battery_layer_update_callback);
	
  layer_add_child(qtp_window_layer, bitmap_layer_get_layer(battery_image_layer));
  layer_add_child(qtp_window_layer, bitmap_layer_get_layer(battery_layer));
	
  GRect dummy_frame = { {0, 0}, {0, 0} };
   day_name_layer = bitmap_layer_create(dummy_frame);
   layer_add_child(qtp_window_layer, bitmap_layer_get_layer(day_name_layer));	
	

  for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    battery_percent_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(qtp_window_layer, bitmap_layer_get_layer(battery_percent_layers[i]));
  }
	
  force_update();

  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, DAY_UNIT);

  update_battery(battery_state_service_peek());
  battery_state_service_subscribe(&update_battery);

}

/* Deallocate QTPlus items when window is hidden */
void qtp_deinit() {
	
	layer_remove_from_parent(bitmap_layer_get_layer(battery_image_layer));
	bitmap_layer_destroy(battery_image_layer);
	
	window_destroy(qtp_window);

}

/* Deallocate persistent QTPlus items when watchface exits */
void qtp_app_deinit() {
	
  accel_tap_service_unsubscribe();
  battery_state_service_unsubscribe();

  text_layer_destroy( qtp_date_layer );

  layer_remove_from_parent(bitmap_layer_get_layer(battery_layer));
  bitmap_layer_destroy(battery_layer);
  gbitmap_destroy(battery_image);
  
 // layer_remove_from_parent(bitmap_layer_get_layer(battery_image_layer));
 // bitmap_layer_destroy(battery_image_layer);

  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
	
for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(battery_percent_layers[i]));
    gbitmap_destroy(battery_percent_image[i]);
    bitmap_layer_destroy(battery_percent_layers[i]); 
  }	

}

/* Create window, layers, text. Display QTPlus */
void qtp_show() {
	qtp_init();
	window_stack_push(qtp_window, true);
	
	//app_timer_cancel(display_timer);
	display_timer = app_timer_register(5000, qtp_timeout, NULL);
	
}

/* Hide QTPlus. Free memory */
void qtp_hide() {
	window_stack_pop(true);
	qtp_deinit();
}

void qtp_set_config(int config) {
	qtp_conf = config;
}

void qtp_set_timeout(int timeout) {
	QTP_WINDOW_TIMEOUT = timeout;	
}

