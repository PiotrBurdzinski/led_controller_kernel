# LED Controller module

This is ONLY the kernel module part, even tho the Kotlin part is very simple it's not stored in the same directory.

## Setup

- Enable module in defconfig (CONFIG_LEDS_CONTROLLER_MODULE=y)
- Make sure the paths to the leds are correct
- Allow app using the module to access /proc/red_blink and /proc/green_blink (If the device is enforcing selinux)
- The files in this repo are meant to be MERGED not replaced with corresponding files in kernel source

## Module configuration

Config is very straight forward and all stored in macros at the top of the leds-controller.c file. </br>
Max brightness is in most cases 13 for mt6360_rgbled but it's value might vary and is stored in /sys/class/leds/<led>/max_brightness. </br>
Delay between on and off is 500ms (LED_BLINK_DELAY). </br>
Paths are very important and can't be set to the symlinks from /sys/class/leds/, instead follow the symlinks and set them to the actual paths </br>

## Example SELinux configuration
For the configuration first a proper error message is needed. </br>
The error in kernel log looks something like this: </br>

    audit(1683729601.608:322): avc: denied
    { open } for comm="test.ledblinker" path="/
    sys/devices/platform/11016000.i2c5/i2c-5/5-0034/mt6360_pmu_rgbled.4.auto/leds/white/brightness" dev="sysfs" ino=37192 scont ext=u:r:untrusted_app: s0: c18, c257, c512, c768 tcontext=u: object_r: sysfs_leds: s0 tclass=file permissive=1 app=com.test.ledblinker

For that the sepolicy would look like:
  
    allow untrusted_app file:sysfs_leds {open read write};
    allow untrusted_app file:proc { write };
    allow untrusted_app sysfs_leds search;
    allow untrusted_app sysfs_leds:file rw_file_perms;
 
 With that config only adapt the type (untrusted_app) for your needs. It will most likely be system_app.
