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
unsigned long lastActivationTime = 0;

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

void button_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  int *pin = (int*)lv_event_get_user_data(e);
  if(code == LV_EVENT_PRESSED) {
    Serial.printf("Button pressed, controlling pin: %d\n", *pin);
    digitalWrite(*pin, HIGH);
  } else if(code == LV_EVENT_RELEASED) {
    Serial.printf("Button released, controlling pin: %d\n", *pin);
    digitalWrite(*pin, LOW);
  }
}

void slider_event_handler(lv_event_t * e) {
  lv_obj_t * slider = (lv_obj_t*)lv_event_get_target(e);
  int *slider_index = (int*)lv_event_get_user_data(e);
  slider_values[*slider_index] = lv_slider_get_value(slider);
  Serial.printf("Slider %d set to %d seconds\n", *slider_index + 1, slider_values[*slider_index]);
}

void create_button(int *pin, const char * label_text, lv_align_t align, int y_offset) {
  lv_obj_t * btn = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_ALL, pin);
  lv_obj_align(btn, align, 0, y_offset);

  lv_obj_t * btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, label_text);
  lv_obj_center(btn_label);
}

void create_slider(int *index, lv_align_t align, int y_offset) {
  lv_obj_t * slider = lv_slider_create(lv_screen_active());
  lv_obj_align(slider, align, 0, y_offset);
  lv_obj_add_event_cb(slider, slider_event_handler, LV_EVENT_VALUE_CHANGED, index);
  lv_slider_set_range(slider, 0, 3600);
}

void lv_create_main_gui(void) {
  static int pin_35 = 35, pin_22 = 22, pin_27 = 27;
  static int slider_0 = 0, slider_1 = 1;

  create_button(&pin_35, "Button 1", LV_ALIGN_CENTER, -50);
  create_button(&pin_22, "Button 2", LV_ALIGN_CENTER, 10);
  create_button(&pin_27, "Button 3", LV_ALIGN_CENTER, 70);
  
  create_slider(&slider_0, LV_ALIGN_CENTER, 130);
  create_slider(&slider_1, LV_ALIGN_CENTER, 180);
}

void setup() {
  Serial.begin(115200);
  int pins[] = {35, 22, 27};
  for (int i = 0; i < 3; i++) {
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

  unsigned long currentTime = millis();
  if (currentTime - lastActivationTime >= 3600000) {  // Every hour
    lastActivationTime = currentTime;
    int pins[] = {35, 22};
    for (int i = 0; i < 2; i++) {
      digitalWrite(pins[i], HIGH);
      delay(slider_values[i] * 1000);
      digitalWrite(pins[i], LOW);
    }
  }
}
