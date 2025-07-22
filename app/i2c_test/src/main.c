#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>


#define I2C_ADDR 0x70
#define RANGE_CMD 0x51
#define I2C_NODE DT_NODELABEL(flexcomm2_lpi2c2)

void main(void)
{
    const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

    uint8_t cmd = RANGE_CMD;
    uint8_t data[2];
    uint16_t distance;
    int ret;

    //initial start up wait
    k_msleep(500); 

    if (!device_is_ready(i2c_dev)) {
        printk("I2C: Device not ready!\n");
        return;
    }
    
    printk("Device ready, starting communication\n");

    while (1) {
        //initiate write to range command
        ret = i2c_write(i2c_dev, &cmd, 1, I2C_ADDR);
        if (ret != 0) {
            printk("I2C: Write to 0x%x failed with error %d\n", I2C_ADDR, ret);
        }

        //wait 80msec after sending range command
        k_msleep(80);

        //initiate read 
        ret = i2c_read(i2c_dev, data, 2, I2C_ADDR);

        if (ret == 0) {
            //read range distance (MSB to LSB)
            distance = (data[0] << 8) | data[1];
            printk("I2C: Range = %d cm\n", distance);
        } else {
            printk("I2C: Read at 0x%x failed with error %d\n", I2C_ADDR, ret);
        }

        //wait 100msec between each read
        k_msleep(100);
    }
}

