#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

int x, y, z;
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

int slider_values[2] = {0, 0};
lv_obj_t *slider_labels[2];

void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  if(touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void slider_event_handler(lv_event_t * e) {
  lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
  int index = *(int*)lv_event_get_user_data(e);
  slider_values[index] = lv_slider_get_value(slider);
  lv_label_set_text_fmt(slider_labels[index], "%d ml", slider_values[index]);
  Serial.printf("Slider %d set to %d ml\n", index + 1, slider_values[index]);
}

void button_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  int *pin = (int*)lv_event_get_user_data(e);
  if(code == LV_EVENT_CLICKED) {
    Serial.printf("Testing valve at pin %d for %d ml\n", *pin, slider_values[*pin == 22 ? 0 : 1]);
    digitalWrite(*pin, HIGH);
    delay(slider_values[*pin == 22 ? 0 : 1]);
    digitalWrite(*pin, LOW);
  }
}

void create_valve_tab(lv_obj_t * parent, const char * label_text, int *slider_index, int pin) {
  lv_obj_t * label = lv_label_create(parent);
  lv_label_set_text_fmt(label, "%s", label_text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t * slider = lv_slider_create(parent);
  lv_slider_set_range(slider, 0, 1000);
  lv_obj_set_width(slider, 180);
  lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 50);
  lv_obj_add_event_cb(slider, slider_event_handler, LV_EVENT_VALUE_CHANGED, slider_index);

  slider_labels[*slider_index] = lv_label_create(parent);
  lv_label_set_text_fmt(slider_labels[*slider_index], "%d ml", slider_values[*slider_index]);
  lv_obj_align(slider_labels[*slider_index], LV_ALIGN_TOP_MID, 0, 90);

  lv_obj_t * freq_label = lv_label_create(parent);
  lv_label_set_text(freq_label, "every week");
  lv_obj_align(freq_label, LV_ALIGN_TOP_MID, 0, 120);

  lv_obj_t * btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 120, 40);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_ALL, &pin);
  lv_obj_t * btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "TEST AMOUNT");
  lv_obj_center(btn_label);
}

void lv_create_main_gui(void) {
  static int pin_22 = 22, pin_27 = 27;
  static int *slider_0 = new int(0);
  static int *slider_1 = new int(1);

  lv_obj_t * tabview = lv_tabview_create(lv_screen_active());
  lv_obj_clear_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);

  // Disable scroll on the tab content as well
  lv_obj_t * tab_content = lv_tabview_get_content(tabview);
  lv_obj_clear_flag(tab_content, LV_OBJ_FLAG_SCROLLABLE);  // Disable all scrolling
  lv_tabview_set_tab_bar_size(tabview, 30);

  lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Valve 1");
  lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Valve 2");

  create_valve_tab(tab1, "Valve 1", slider_0, pin_22); // using pointer for slider index
  create_valve_tab(tab2, "Valve 2", slider_1, pin_27); // using pointer for slider index
}

void setup() {
  Serial.begin(115200);
  int pins[] = {22, 27};
  for (int i = 0; i < 2; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }

  lv_init();
  lv_log_register_print_cb(log_print);
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2);

  lv_display_t * disp;
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  lv_create_main_gui();
}

void loop() {
  lv_task_handler();
  lv_tick_inc(5);
  delay(5);
}
