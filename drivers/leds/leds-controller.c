#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define LED_RED_NODE_FILE "/sys/devices/platform/11016000.i2c5/i2c-5/5-0034/mt6360_pmu_rgbled.4.auto/leds/red/brightness"
#define LED_GREEN_NODE_FILE "/sys/devices/platform/11016000.i2c5/i2c-5/5-0034/mt6360_pmu_rgbled.4.auto/leds/green/brightness"
#define LED_MAX_BRIGHTNESS "13"
#define LED_BLINK_DELAY 500

static unsigned int red_blink = 0, green_blink = 0;

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

static void led_controller_blink(size_t led)
{
    int i = 0, blinks = red_blink;
    const char* path = LED_RED_NODE_FILE;
    if (led == 1){
        path = LED_GREEN_NODE_FILE;
        blinks = green_blink;
    }
    
    for(i; i < blinks; i++){
        write_to_file(path, LED_MAX_BRIGHTNESS, 3);
        msleep(LED_BLINK_DELAY);
        write_to_file(path, "0", 3);
        if (i < blinks)
            msleep(LED_BLINK_DELAY);
    }
}

static void led_controller_alert(){
    write_to_file(LED_RED_NODE_FILE, LED_MAX_BRIGHTNESS, 3);
    msleep(LED_BLINK_DELAY);
    write_to_file(LED_RED_NODE_FILE, "0", 3);
    write_to_file(LED_GREEN_NODE_FILE, LED_MAX_BRIGHTNESS, 3);
    msleep(LED_BLINK_DELAY);
    write_to_file(LED_GREEN_NODE_FILE, "0", 3);
}

static int red_blink_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%u\n", red_blink);
    return 0;
}

static int red_blink_open(struct inode *inode, struct file *file)
{
    return single_open(file, red_blink_show, inode->i_private);
}

static ssize_t red_blink_write(struct file *file, const char __user *buffer,
    size_t count, loff_t *ppos)
{
    int val;
    if (kstrtoint_from_user(buffer, count, 10, &val) != 0) {
        return -EINVAL;
    }
    if (val > 0){
        red_blink = val;
        led_controller_blink(0);
    } else {
        led_controller_alert();
    }
    return count;
}

static int green_blink_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%u\n", green_blink);
    return 0;
}

static int green_blink_open(struct inode *inode, struct file *file)
{
    return single_open(file, green_blink_show, inode->i_private);
}

static ssize_t green_blink_write(struct file *file, const char __user *buffer,
    size_t count, loff_t *ppos)
{
    int val;
    if (kstrtoint_from_user(buffer, count, 10, &val) != 0) {
        return -EINVAL;
    }
    green_blink = val;
    led_controller_blink(1);
    return count;
}

static const struct file_operations red_blink_fops = {
    .owner = THIS_MODULE,
    .open = red_blink_open,
    .write = red_blink_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static const struct file_operations green_blink_fops = {
    .owner = THIS_MODULE,
    .open = green_blink_open,
    .write = green_blink_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init blinks_init(void)
{
    proc_create("red_blink", 0666, NULL, &red_blink_fops);
    proc_create("green_blink", 0666, NULL, &green_blink_fops);
    return 0;
}

static void __exit blinks_exit(void)
{
    remove_proc_entry("red_blink", NULL);
    remove_proc_entry("green_blink", NULL);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Piotr Burdzinski");
MODULE_DESCRIPTION("LED Blinking module for mt6360_pmu_rgb led");

module_init(blinks_init);
module_exit(blinks_exit);
