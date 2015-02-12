#include <pebble.h>

// Config
#define QTP_K_SHOW_TIME 1
#define QTP_K_AUTOHIDE 4
#define QTP_K_INVERT 16
#define QTP_K_SUBSCRIBE 32
#define QTP_K_VIBRATE 64

// Items
static int QTP_WINDOW_TIMEOUT = 2000;
static Window *qtp_window;
static bool qtp_is_showing;
static int qtp_conf;

// Methods
void qtp_setup();
void qtp_app_deinit();

void qtp_show();
void qtp_hide();
void qtp_timeout();

void qtp_tap_handler(AccelAxisType axis, int32_t direction);
void qtp_click_config_provider(Window *window);
void qtp_back_click_responder(ClickRecognizerRef recognizer, void *context);

void qtp_bluetooth_callback(bool connected);

void qtp_update_battery_status(bool mark_dirty);
void qtp_update_bluetooth_status(bool mark_dirty);

void qtp_init();
void qtp_deinit();

// Helpers
bool qtp_is_show_time();
bool qtp_is_autohide();
bool qtp_is_invert();
bool qtp_should_vibrate();

void qtp_set_config(int config);
void qtp_set_timeout(int timeout);
void qtp_init_bluetooth_status(bool status);