#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/string.h>


#define LED_RED_NODE_FILE "/sys/devices/platform/11016000.i2c5/i2c-5/5-0034/mt6360_pmu_rgbled.4.auto/leds/red/brightness"
#define LED_GREEN_NODE_FILE "/sys/devices/platform/11016000.i2c5/i2c-5/5-0034/mt6360_pmu_rgbled.4.auto/leds/green/brightness"

#define LED_BLINK_DELAY 400
#define LED_MAX_BRIGHTNESS "13"

static char red_value[32] = "0";
static char green_value[32] = "0";
static char alert_value[32] = "0";

void write_to_file(const char* path, const char* data, size_t size) {
    struct file* filep;
    mm_segment_t old_fs;
    int ret;

    filep = filp_open(path, O_WRONLY | O_CREAT, 0644);
    if (!filep || IS_ERR(filep)) {
        printk(KERN_ERR "Failed to open file %s for writing\n", path);
        return;
    }

    old_fs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(filep, data, size, &filep->f_pos);
    if (ret < 0) {
        printk(KERN_ERR "Failed to write to file %s\n", path);
    }

    set_fs(old_fs);
    filp_close(filep, NULL);
}

static ssize_t led_controller_set_brightness(size_t led, const char *buf, size_t count)
{
    int i;
    const char* path = LED_RED_NODE_FILE;
    long blinks;
    if (kstrtol(buf, 10, &blinks))
        return -EINVAL;
    if (led == 1){
        path = LED_GREEN_NODE_FILE;
    }
    
    for(i = 0; i < blinks; i++){
        write_to_file(path, LED_MAX_BRIGHTNESS, sizeof(buf));
        msleep(500);
        write_to_file(path, "0", sizeof(buf));
        if (i < blinks)
            msleep(500);
    }
    return count;
}

static void led_alert(){
    write_to_file(LED_RED_NODE_FILE, LED_MAX_BRIGHTNESS, 3);
    msleep(LED_BLINK_DELAY);
    write_to_file(LED_RED_NODE_FILE, "0", 3);
    write_to_file(LED_GREEN_NODE_FILE, LED_MAX_BRIGHTNESS, 3);
    msleep(LED_BLINK_DELAY);
    write_to_file(LED_GREEN_NODE_FILE, "0", 3);
}

static ssize_t red_blinks_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    ssize_t ret;

    ret = led_controller_set_brightness(0, buf, count);
    if (ret < 0) {
        return ret;
    }

    strncpy(red_value, buf, sizeof(red_value));
    return count;
}

static ssize_t green_blinks_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    ssize_t ret;

    ret = led_controller_set_brightness(1, buf, count);
    if (ret < 0) {
        return ret;
    }

    strncpy(green_value, buf, sizeof(green_value));
    return count;
}

static ssize_t led_alert_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    led_alert(); led_alert();
    strncpy(green_value, buf, sizeof(green_value));
    return count;
}

static ssize_t red_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s\n", red_value);
}

static ssize_t green_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s\n", green_value);
}

static ssize_t alert_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s\n", alert_value);
}

static struct kobj_attribute led_controller_red_attr =
    __ATTR(red_blinks, 0755, red_show, red_blinks_store);

static struct kobj_attribute led_controller_green_attr =
    __ATTR(green_blinks, 0755, green_show, green_blinks_store);

static struct kobj_attribute led_controller_alert_attr =
    __ATTR(led_alert, 0755, alert_show, led_alert_store);

static struct attribute *led_controller_attrs[] = {
    &led_controller_red_attr.attr,
    &led_controller_green_attr.attr,
    &led_controller_alert_attr.attr,
    NULL,
};

static struct attribute_group led_controller_attr_group = {
    .attrs = led_controller_attrs,
};

static struct kobject *led_controller_kobj;

static int __init led_controller_init(void)
{
    int ret;

    led_controller_kobj = kobject_create_and_add("led_controller", NULL);
    if (!led_controller_kobj) {
        pr_err("Failed to create sysfs directory\n");
        return -ENOMEM;
    }

    ret = sysfs_create_group(led_controller_kobj, &led_controller_attr_group);
    if (ret) {
        pr_err("Failed to create sysfs files\n");
        kobject_put(led_controller_kobj);
        return ret;
    }

    return 0;
}

static void __exit led_controller_exit(void)
{
    sysfs_remove_group(led_controller_kobj, &led_controller_attr_group);
    kobject_put(led_controller_kobj);
}

module_init(led_controller_init);
module_exit(led_controller_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Piotr Burdzinski");
MODULE_DESCRIPTION("LED Blinking module for mt6360_pmu_rgb led");
