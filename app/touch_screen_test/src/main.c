#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <lvgl_input_device.h>
#include <zephyr/sys/util.h>
#include <zephyr/input/input.h>


LOG_MODULE_REGISTER(touch_test, LOG_LEVEL_INF);

#if !DT_NODE_EXISTS(DT_CHOSEN(zephyr_touch))
#error "Unsupported board: zephyr,touch is not assigned"
#endif

#if !DT_NODE_EXISTS(DT_CHOSEN(zephyr_display))
#error "Unsupported board: zephyr,display is not assigned"
#endif

static struct {
	size_t x;
	size_t y;
	bool pressed;
} touch_point;

static void input_event_callback(struct input_event *evt, void *user_data)
{
    if (evt->type == INPUT_EV_ABS) {
        if (evt->code == INPUT_ABS_X) {
            touch_point.x = evt->value;
        }
        else if (evt->code == INPUT_ABS_Y){
            touch_point.y = evt->value;
        }
    } else if (evt->type == INPUT_EV_KEY && evt->code == INPUT_BTN_TOUCH) {
        touch_point.pressed = evt->value;
    }
}



// LVGL input device callback
void my_input_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    if(touch_point.pressed) {
        data->point.x = touch_point.x;
        data->point.y = touch_point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void touch_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), 0); // Red
        lv_label_set_text(label, "Touched!");
    } else if (code == LV_EVENT_RELEASED) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x3E9440), 0); // Green
        lv_label_set_text(label, "Touch me");
    }
}


int main(void)
{
    static const struct device *const touch_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_touch));
    static const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return 0;
    }

    INPUT_CALLBACK_DEFINE(touch_dev, input_event_callback, NULL);


    //input device connected to display
    lv_indev_t * indev = lv_indev_create();
    //touch pad pointer device 
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    //driver function
    lv_indev_set_read_cb(indev, my_input_read);

    lv_obj_t *btn = lv_button_create(lv_screen_active());
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, touch_btn_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn, touch_btn_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Touch me");
    lv_obj_center(label);

    // Turn on backlight and start drawing                                                                               
    lv_timer_handler();
    display_blanking_off(display_dev);

    while (1) {
        lv_timer_handler();  
        k_sleep(K_MSEC(10));
    }
}



