#include <pebble.h>

#define DICTATION_BUFFER_SIZE 1024

static Window *s_main_window;
static TextLayer *s_prompt_layer;
static TextLayer *s_status_layer;
static DictationSession *s_dictation_session;
static char s_transcription_buffer[DICTATION_BUFFER_SIZE];
// No JS-ready handshake needed — AppMessage queues outbound messages
// until the JS layer is ready to receive them.

// AppMessage keys (must match package.json messageKeys order)
enum {
  KEY_TRANSCRIPTION = 0,
  KEY_STATUS,
  KEY_CONFIG_URL,
  KEY_CONFIG_METHOD,
  KEY_CONFIG_HEADERS,
  KEY_CONFIG_BODY
};

static void update_status(const char *text) {
  text_layer_set_text(s_status_layer, text);
}

static void send_transcription(const char *text) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    update_status("Send failed");
    return;
  }

  dict_write_cstring(iter, KEY_TRANSCRIPTION, text);
  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    update_status("Send failed");
  } else {
    update_status("Sending...");
  }
}

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status,
                                       char *transcription, void *context) {
  if (status == DictationSessionStatusSuccess) {
    snprintf(s_transcription_buffer, sizeof(s_transcription_buffer), "%s", transcription);
    text_layer_set_text(s_prompt_layer, s_transcription_buffer);
    send_transcription(s_transcription_buffer);
  } else {
    static char s_error_buf[64];
    snprintf(s_error_buf, sizeof(s_error_buf), "Dictation error: %d", (int)status);
    update_status(s_error_buf);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  update_status("Listening...");
  text_layer_set_text(s_prompt_layer, "Speak now...");
  dictation_session_start(s_dictation_session);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *status_tuple = dict_find(iter, KEY_STATUS);
  if (status_tuple) {
    int status_val = status_tuple->value->int32;
    if (status_val == 200) {
      update_status("Sent!");
      vibes_short_pulse();
    } else {
      static char s_err_buf[48];
      snprintf(s_err_buf, sizeof(s_err_buf), "HTTP error: %d", status_val);
      update_status(s_err_buf);
      vibes_double_pulse();
    }
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  update_status("Message dropped");
}

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
  // Message delivered to phone successfully
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  update_status("Outbox failed");
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Prompt text layer (main area)
  s_prompt_layer = text_layer_create(GRect(10, 30, bounds.size.w - 20, bounds.size.h - 70));
  text_layer_set_background_color(s_prompt_layer, GColorClear);
  text_layer_set_text_color(s_prompt_layer, GColorWhite);
  text_layer_set_font(s_prompt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_prompt_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_prompt_layer, GTextOverflowModeTrailingEllipsis);
  text_layer_set_text(s_prompt_layer, "Press SELECT\nto dictate");
  layer_add_child(window_layer, text_layer_get_layer(s_prompt_layer));

  // Status text layer (bottom)
  s_status_layer = text_layer_create(GRect(10, bounds.size.h - 35, bounds.size.w - 20, 30));
  text_layer_set_background_color(s_status_layer, GColorClear);
  text_layer_set_text_color(s_status_layer, GColorLightGray);
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_text(s_status_layer, "Ready \xe2\x80\x94 press SELECT");
  layer_add_child(window_layer, text_layer_get_layer(s_status_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_prompt_layer);
  text_layer_destroy(s_status_layer);
}

static void init(void) {
  // Register AppMessage handlers
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);

  // Open AppMessage — inbox 2048 for config responses, outbox 1024 for long transcriptions
  app_message_open(2048, 1024);

  // Create dictation session
  s_dictation_session = dictation_session_create(DICTATION_BUFFER_SIZE,
                                                  dictation_session_callback, NULL);

  // Create main window
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  dictation_session_destroy(s_dictation_session);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
