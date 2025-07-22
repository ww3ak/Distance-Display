#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h> 
#include <zephyr/sys/printk.h>    

#define MB7040_NODE DT_NODELABEL(mb70401)

int main(void) 
{
    struct sensor_value sensor_val; 
    int ret;

	const struct device *const sensor_dev = DEVICE_DT_GET_ONE(maxbotix_mb7040);

	//initial start up wait
    k_msleep(500); 

    if (!device_is_ready(sensor_dev)) {
        printk("Device not ready\n");
        return -1;
    }

    printk("Device ready, starting communication\n");

    while (1){

        //initiiate sample fetch 
        ret = sensor_sample_fetch(sensor_dev);
        if (ret != 0) {
            printk("ERROR: Failed to fetch sample: %d\n", ret);
        }

        //continuously try to get measurment untl its ready 
        do {
            ret = sensor_channel_get(sensor_dev, SENSOR_CHAN_DISTANCE, &sensor_val);
            if (ret == -EAGAIN) {
                //short wait for things to catch up
                k_msleep(5);
            }

        } while (ret != 0);

        if (ret != 0) {
            printk("ERROR: Failed to get channel: %d\n", ret);
        } else {
            printk("Distance: %d.%06d\n", sensor_val.val1, sensor_val.val2);
        }

    }

    return 0;
}