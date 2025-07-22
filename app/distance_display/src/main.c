#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>
#include <zephyr/sys/printk.h>    
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h> 

LOG_MODULE_REGISTER(distance_display, LOG_LEVEL_INF);


#define MAX_VALUE 765
#define MIN_VALUE 0
#define MAX_POINTS 50

static lv_obj_t *history_screen;
static lv_obj_t *main_screen;

static lv_obj_t *label;
static lv_obj_t *sw;
static lv_obj_t *cm_label;
static lv_obj_t *inch_label;
static lv_obj_t *bar;
static lv_obj_t *title;
static lv_obj_t *chart;
static lv_obj_t *pause_btn;
static lv_obj_t *darkMode_btn;

static lv_style_t default_style;
static lv_chart_series_t *series;
static bool chart_paused;
static struct sensor_value sensor_val;
static lv_style_t style_bar_indic;
static bool use_cm;

static bool is_dark_mode;
static lv_style_t dark_bg_style;
static lv_style_t light_bg_style;
static lv_style_t dark_btn_style;
static lv_style_t light_btn_style;
static lv_style_t dark_label_style;
static lv_style_t light_label_style;

static int saved_points[MAX_POINTS];
static int saved_count = 0;
static lv_obj_t *save_btn;
static lv_obj_t *history_btn;
static lv_obj_t *hist_title;
static lv_obj_t *point_label;


void history_points(void);

static int update_distance(const struct device *sensor_dev)
{
    int ret;
    char buf[8];

    ret = sensor_sample_fetch(sensor_dev);
    if (ret != 0) {
        printk("ERROR: Failed to fetch sample: %d\n", ret);
        return ret;
    }

    do {
        ret = sensor_channel_get(sensor_dev, SENSOR_CHAN_DISTANCE, &sensor_val);
        if (ret == -EAGAIN) {
            k_msleep(5);
        }
    } while (ret == -EAGAIN);

    if (ret != 0) {
        printk("ERROR: Failed to get channel: %d\n", ret);
        return ret;
    }
    // Convert to total centimeters from meters + micro-meters
    int total_cm = sensor_val.val1 * 100 + sensor_val.val2 / 10000;

    // Clamp value if needed
    if (total_cm < MIN_VALUE) total_cm = MIN_VALUE;
    if (total_cm > MAX_VALUE) total_cm = MAX_VALUE;

    // Print distance

    if (use_cm){
        // LOG_INF("Distance in cm: %d", total_cm);

        lv_snprintf(buf, sizeof(buf), "%d cm", total_cm);
    } else{
        //convert cm to inches 
        int inches = total_cm / 2.54;
        // LOG_INF("Distance in inches: %d", inches);

        lv_snprintf(buf, sizeof(buf), "%d ''", inches);
    }

    lv_chart_set_next_value(chart, series, total_cm);

        // Update LVGL UI
    lv_label_set_text(label, buf);
    lv_bar_set_value(bar, total_cm, LV_ANIM_ON);


    return 0;
}

static void sw_event_cb(lv_event_t * e){
    lv_obj_t * sw = lv_event_get_target_obj(e);

    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        use_cm = true;
    } else {
        use_cm = false;
    }
}


void init_styles(void) {
    // Backgrounds
    lv_style_init(&dark_bg_style);
    lv_style_set_bg_color(&dark_bg_style, lv_color_hex(0x121212));
    lv_style_set_bg_opa(&dark_bg_style, LV_OPA_COVER);

    lv_style_init(&light_bg_style);
    lv_style_set_bg_color(&light_bg_style, lv_color_hex(0xf0f0f0));
    lv_style_set_bg_opa(&light_bg_style, LV_OPA_COVER);

    // Buttons
    lv_style_init(&dark_btn_style);
    lv_style_set_bg_color(&dark_btn_style, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_text_color(&dark_btn_style, lv_color_white());

    lv_style_init(&light_btn_style);
    lv_style_set_bg_color(&light_btn_style, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
    lv_style_set_text_color(&light_btn_style, lv_color_black());

    // Labels
    lv_style_init(&dark_label_style);
    lv_style_set_text_color(&dark_label_style, lv_color_white());

    lv_style_init(&light_label_style);
    lv_style_set_text_color(&light_label_style, lv_color_black());
}


void unit_swtich(void){

    sw = lv_switch_create(lv_screen_active());
    lv_obj_align(sw, LV_ALIGN_TOP_LEFT, 95, 10);
    lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, sw_event_cb, LV_EVENT_VALUE_CHANGED, label);

    cm_label = lv_label_create(lv_scr_act());
    lv_label_set_text(cm_label, "cm");
    lv_obj_align_to(cm_label, sw, LV_ALIGN_TOP_LEFT, 60, 10);
    lv_obj_set_style_text_font(cm_label, &lv_font_montserrat_20, 0);

    inch_label = lv_label_create(lv_scr_act());
    lv_label_set_text(inch_label, "inches");
    lv_obj_align_to(inch_label, sw, LV_ALIGN_TOP_LEFT, -75, 10);
    lv_obj_set_style_text_font(inch_label, &lv_font_montserrat_20, 0);
}

void pause_btn_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        chart_paused = !chart_paused; 

        static lv_style_t red_style;
        static bool styles_initialized = false;

        if (!styles_initialized) {
            lv_style_init(&red_style);
            lv_style_set_bg_color(&red_style, lv_palette_main(LV_PALETTE_RED));
            lv_style_set_bg_opa(&red_style, LV_OPA_COVER);

            styles_initialized = true;
        }

        lv_obj_t *btn = lv_event_get_target_obj(e);

        if (chart_paused) {
            lv_obj_add_style(btn, &red_style, LV_PART_MAIN);
        } else {
            lv_obj_add_style(btn, &default_style, LV_PART_MAIN);
            lv_obj_remove_style(btn, &red_style, LV_PART_MAIN);
        }
    }
}

void pause_button(void){

    lv_style_init(&default_style);
    lv_style_set_bg_color(&default_style, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_bg_opa(&default_style, LV_OPA_COVER);

    pause_btn = lv_button_create(lv_screen_active());
    lv_obj_align(pause_btn, LV_ALIGN_TOP_RIGHT, -20, 5);
    lv_obj_set_size(pause_btn, 100, 50);
    lv_obj_add_event_cb(pause_btn, pause_btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_style(pause_btn, &default_style, LV_PART_MAIN);

    lv_obj_t * pause = lv_label_create(pause_btn);         
    lv_label_set_text(pause, "Pause");  
    lv_obj_center(pause);
    lv_obj_set_style_text_font(pause, &lv_font_montserrat_20, 0);
}

void set_theme(bool dark) {
    if (dark) {
        lv_obj_add_style(lv_scr_act(), &dark_bg_style, 0);
        lv_obj_remove_style(lv_scr_act(), &light_bg_style, 0);

        lv_obj_add_style(darkMode_btn, &dark_btn_style, LV_PART_MAIN);

        lv_obj_add_style(label, &dark_label_style, 0);
        lv_obj_add_style(title, &dark_label_style, 0);
        lv_obj_add_style(cm_label, &dark_label_style, 0);
        lv_obj_add_style(inch_label, &dark_label_style, 0);

        lv_chart_set_series_color(chart, series, lv_palette_main(LV_PALETTE_ORANGE));


    } else {
        lv_obj_add_style(lv_scr_act(), &light_bg_style, 0);
        lv_obj_remove_style(lv_scr_act(), &dark_bg_style, 0);

        lv_obj_add_style(darkMode_btn, &light_btn_style, LV_PART_MAIN);

        lv_obj_add_style(label, &light_label_style, 0);
        lv_obj_add_style(title, &light_label_style, 0);
        lv_obj_add_style(cm_label, &light_label_style, 0);
        lv_obj_add_style(inch_label, &light_label_style, 0);


        lv_chart_set_series_color(chart, series, lv_palette_main(LV_PALETTE_BLUE));

    }
}


static void dark_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        is_dark_mode = !is_dark_mode;
        set_theme(is_dark_mode);
    }
}

void dark_mode(void){
    darkMode_btn = lv_button_create(lv_scr_act());
    lv_obj_align(darkMode_btn, LV_ALIGN_TOP_RIGHT, -140, 5);
    lv_obj_set_size(darkMode_btn, 130, 50);
    lv_obj_add_event_cb(darkMode_btn, dark_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(darkMode_btn);
    lv_label_set_text(label, "Dark Mode");
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
}

void distance_bar(void)
{
    lv_style_init(&style_bar_indic);
    lv_style_set_bg_opa(&style_bar_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_bar_indic, lv_color_make(255, 0, 0));
    lv_style_set_bg_grad_color(&style_bar_indic, lv_color_make(0, 0, 255));
    lv_style_set_bg_grad_dir(&style_bar_indic, LV_GRAD_DIR_HOR);

    title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "Distance Monitor");
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    bar = lv_bar_create(lv_scr_act());
    lv_bar_set_range(bar, MIN_VALUE, MAX_VALUE);
    lv_obj_set_size(bar, 400, 30);
    lv_obj_align_to(bar, chart, LV_ALIGN_CENTER, 0, 60);

    lv_obj_add_style(bar, &style_bar_indic, LV_PART_INDICATOR);
    lv_obj_set_style_anim_time(bar, 300, LV_PART_INDICATOR);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "0");
    lv_obj_align_to(label, bar, LV_ALIGN_OUT_BOTTOM_MID, -20, 5);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0); 
}


void distance_chart(void){
    chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, 400, 120);
    lv_obj_align_to(chart, bar, LV_ALIGN_OUT_TOP_MID, 0, -10);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, MIN_VALUE, MAX_VALUE);
    lv_chart_set_point_count(chart, 50);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
}

void save_current_point(void) {
    if (saved_count >= MAX_POINTS) {
        return; // Array is full
    }
    
    // Convert sensor value to cm (same logic as in update_distance)
    int total_cm = sensor_val.val1 * 100 + sensor_val.val2 / 10000;
    
    // Clamp value if needed
    if (total_cm < MIN_VALUE) total_cm = MIN_VALUE;
    if (total_cm > MAX_VALUE) total_cm = MAX_VALUE;
    
    // Save the current distance
    saved_points[saved_count] = total_cm;
    saved_count++;
}


void back_to_main_cb(lv_event_t * e) {
    lv_scr_load(main_screen);   // Switch back to main screen

    if (history_screen != NULL) {
        lv_obj_del(history_screen);  // Delete history UI elements
        history_screen = NULL;
    }
}


void reset_cb(lv_event_t * e) {
    // Clear saved points
    for (int i = 0; i < saved_count; i++) {
        saved_points[i] = 0;
    }
    saved_count = 0;

    history_points();
}

void history_points(void) {
    history_screen = lv_obj_create(NULL);  // Create a new screen

    // Set background color based on theme
    if (is_dark_mode) {
        lv_obj_set_style_bg_color(history_screen, lv_color_hex(0x121212), 0);
        // lv_obj_add_style(hist_title, &dark_label_style, 0);
        // lv_obj_add_style(point_label, &dark_label_style, 0);

    } else {
        lv_obj_set_style_bg_color(history_screen, lv_color_hex(0xf0f0f0), 0);
        // lv_obj_add_style(hist_title, &light_label_style, 0);
        // lv_obj_add_style(point_label, &light_label_style, 0);
    }

    // Add a title label
    hist_title = lv_label_create(history_screen);
    lv_label_set_text(hist_title, "Saved Points History");
    lv_obj_align(hist_title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(hist_title, &lv_font_montserrat_20, 0); 

    if (is_dark_mode) {
        lv_obj_add_style(hist_title, &dark_label_style, 0);

    } else {
        lv_obj_add_style(hist_title, &light_label_style, 0);
    }

    // Show all saved points
    for (int i = 0; i < saved_count; i++) {
        char buf[32];

        if (use_cm) {
            lv_snprintf(buf, sizeof(buf), "Point %d: %d cm", i + 1, saved_points[i]);
        } else {
            int inches = saved_points[i] / 2.54;
            lv_snprintf(buf, sizeof(buf), "Point %d: %d ''", i + 1, inches);
        }

        point_label = lv_label_create(history_screen);
        lv_label_set_text(point_label, buf);
        lv_obj_align(point_label, LV_ALIGN_TOP_LEFT, 10, 40 + i * 20);

        if (is_dark_mode) {
            lv_obj_add_style(point_label, &dark_label_style, 0);

        } else {
            lv_obj_add_style(point_label, &light_label_style, 0);
        }
    }

    // Create a "Back" button
    lv_obj_t *back_btn = lv_button_create(history_screen);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -10, 30);
    lv_obj_add_event_cb(back_btn, back_to_main_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);

    //create reset button
    lv_obj_t *reset_btn = lv_button_create(history_screen);
    lv_obj_set_size(reset_btn, 100, 40);
    lv_obj_align(reset_btn, LV_ALIGN_RIGHT_MID, -10, -10);
    lv_obj_add_event_cb(reset_btn, reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "Reset");
    lv_obj_center(reset_label);

    // Load the history screen
    lv_scr_load(history_screen);
}



void save_btn_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        save_current_point();
    }
}

void history_btn_event_cb(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        history_points();
    }
}

void create_save_history_buttons(void) {
    save_btn = lv_button_create(lv_scr_act());
    lv_obj_set_size(save_btn, 80, 40);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(save_btn, save_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_center(save_label);

    history_btn = lv_button_create(lv_scr_act());
    lv_obj_set_size(history_btn, 80, 40);
    lv_obj_align(history_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(history_btn, history_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * load_label = lv_label_create(history_btn);
    lv_label_set_text(load_label, "History");
    lv_obj_center(load_label);
}


int main(void)
{
    const struct device *sensor_dev = DEVICE_DT_GET_ONE(maxbotix_mb7040);
    k_msleep(500);

    if (!device_is_ready(sensor_dev)) {
        printk("Device not ready\n");
        return -1;
    }
    main_screen = lv_scr_act();
    init_styles();

    use_cm = true;
    chart_paused = false;
    is_dark_mode = false;
    distance_bar();
    unit_swtich();
    distance_chart();
    pause_button();
    dark_mode();
    set_theme(false);
    create_save_history_buttons();
    while (1) {
        if (!chart_paused) {
            update_distance(sensor_dev);
        }
        lv_timer_handler();
        k_sleep(K_MSEC(50)); 
    }

    return 0;
}