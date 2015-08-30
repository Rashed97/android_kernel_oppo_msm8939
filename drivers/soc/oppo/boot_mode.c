#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <soc/qcom/smem.h>
#include <asm/uaccess.h>

#include <linux/sysfs.h>
#include <linux/gpio.h>
#include <soc/oppo/boot_mode.h>

static struct kobject *systeminfo_kobj;

static int ftm_mode = MSM_BOOT_MODE__NORMAL;

int __init board_mfg_mode_init(void)
{
	char *substr;

	substr = strstr(boot_command_line, "oppo_ftm_mode=");
	if (substr) {
		substr += strlen("oppo_ftm_mode=");

		if (strncmp(substr, "factory2", 5) == 0)
			ftm_mode = MSM_BOOT_MODE__FACTORY;
		else if (strncmp(substr, "ftmwifi", 5) == 0)
			ftm_mode = MSM_BOOT_MODE__WLAN;
		else if (strncmp(substr, "ftmmos", 5) == 0)
			ftm_mode = MSM_BOOT_MODE__MOS;
		else if (strncmp(substr, "ftmrf", 5) == 0)
			ftm_mode = MSM_BOOT_MODE__RF;
		else if (strncmp(substr, "ftmrecovery", 5) == 0)
			ftm_mode = MSM_BOOT_MODE__RECOVERY;
		else if (strncmp(substr, "ftmsilence", 10) == 0)
			ftm_mode = MSM_BOOT_MODE__SILENCE;
	}

	pr_info("%s: oppo_ftm_mode is %s\n", __func__, ftm_mode);

	return 0;
}
//__setup("oppo_ftm_mode=", board_mfg_mode_init);

int get_boot_mode(void)
{
	return ftm_mode;
}

static ssize_t ftmmode_show(struct kobject *kobj, struct kobj_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "%d\n", ftm_mode);
}

struct kobj_attribute ftmmode_attr = {
	.attr = { "ftmmode", 0644 },
	.show = &ftmmode_show,
};

#define mdm_drv_ap2mdm_pmic_pwr_en_gpio  27

static ssize_t closemodem_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	/* write '1' to close and '0' to open */
	//pr_debug("closemodem buf[0] = 0x%x", buf[0]);
	switch (buf[0]) {
	case 0x30:
		break;
	case 0x31:
		pr_info("Closing modem");
		gpio_direction_output(mdm_drv_ap2mdm_pmic_pwr_en_gpio, 0);
		mdelay(4000);
		break;
	default:
		break;
	}

	return count;
}

struct kobj_attribute closemodem_attr = {
	.attr = { "closemodem", 0644 },
	.store = &closemodem_store
};

static struct attribute * g[] = {
	&ftmmode_attr.attr,
	&closemodem_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};

char pwron_event[16];
static int __init start_reason_init(void)
{
	int i;
	char * substr = strstr(boot_command_line, "androidboot.startupmode=");

	if (NULL == substr)
		return 0;

	substr += strlen("androidboot.startupmode=");

	for (i = 0; substr[i] != ' '; i++) {
		pwron_event[i] = substr[i];
	}
	pwron_event[i] = '\0';

	pr_info("%s: androidboot.startupmode is %s\n", __func__, pwron_event);

	return 1;
}
//__setup("androidboot.startupmode=", start_reason_setup);

char boot_mode[16];
static int __init boot_mode_init(void)
{
	int i;
	int rc = 0;
	char *substr = strstr(boot_command_line, "androidboot.mode=");

	if (NULL == substr)
		return 0;

	substr += strlen("androidboot.mode=");

	for (i = 0; substr[i] != ' '; i++) {
		boot_mode[i] = substr[i];
	}
	boot_mode[i] = '\0';

	pr_info("%s: androidboot.mode is %s\n", __func__, boot_mode);

	board_mfg_mode_init();
	start_reason_init();

	systeminfo_kobj = kobject_create_and_add("systeminfo", NULL);
	if (systeminfo_kobj)
		rc = sysfs_create_group(systeminfo_kobj, &attr_group);

	return 1;
}
//__setup("androidboot.mode=", boot_mode_setup);

arch_initcall(boot_mode_init);
