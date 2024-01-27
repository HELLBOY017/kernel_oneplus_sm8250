#define LOG_TAG         "Sysfs"

#include "cts_config.h"
#include "cts_core.h"
#include "cts_test.h"
#include "cts_sfctrl.h"
#include "cts_spi_flash.h"
#include "cts_strerror.h"

#ifdef CONFIG_CTS_SYSFS

#define SPLIT_LINE_STR \
    "-----------------------------------------------------------------------------------------------\n"
#define ROW_NUM_FORMAT_STR  "%2d | "
#define COL_NUM_FORMAT_STR  "%4u "
#define DATA_FORMAT_STR     "%5d"


#define DIFFDATA_BUFFER_SIZE(cts_dev) \
        (cts_dev->fwdata.rows * cts_dev->fwdata.cols * 2)

#define MAX_ARG_NUM                 (100)
#define MAX_ARG_LENGTH              (1024)

static char cmdline_param[MAX_ARG_LENGTH + 1];
int  argc;
char *argv[MAX_ARG_NUM];

static int jitter_test_frame = 10;
static s16 *manualdiff_base = NULL;

int parse_arg(const char *buf, size_t count)
{
    char *p;

    memcpy(cmdline_param, buf, min((size_t)MAX_ARG_LENGTH, count));
    cmdline_param[count] = '\0';

    argc = 0;
    p = strim(cmdline_param);
    if (p == NULL || p[0] == '\0') {
        return 0;
    }

    while (p && p[0] != '\0' && argc < MAX_ARG_NUM) {
        argv[argc++] = strsep(&p, " ,");
    }

    return argc;
}

/* echo addr value1 value2 value3 ... valueN > write_reg */
static ssize_t write_firmware_register_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u16 addr;
    int i, ret;
    u8 *data = NULL;

    parse_arg(buf, count);

    TPD_INFO("<I> Write firmware register '%.*s'\n", (int)count, buf);

    if (argc < 2) {
        TPD_INFO("<E> Too few args %d\n", argc);
        return -EFAULT;
    }

    ret = kstrtou16(argv[0], 0, &addr);
    if (ret) {
        TPD_INFO("<E> Invalid address %s\n", argv[0]);
        return -EINVAL;
    }

    data = (u8 *)kmalloc(argc - 1, GFP_KERNEL);
    if (data == NULL) {
        TPD_INFO("<E> Allocate buffer for write data failed\n");
        return -ENOMEM;
    }

    for (i = 1; i < argc; i++) {
        ret = kstrtou8(argv[i], 0, data + i - 1);
        if (ret) {
            TPD_INFO("<E> Invalid value %s\n", argv[i]);
            goto free_data;
        }
    }

    ret = cts_fw_reg_writesb(cts_dev, addr, data, argc - 1);
    if (ret) {
        TPD_INFO("<E> Write firmware register addr: 0x%04x size: %d failed\n",
            addr, argc - 1);
        goto free_data;
    }

free_data:
    kfree(data);

    return (ret < 0 ? ret : count);
}
static DEVICE_ATTR(write_reg, S_IWUSR, NULL, write_firmware_register_store);

static ssize_t read_firmware_register_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#define PRINT_ROW_SIZE          (16)
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u16 addr, size, i, remaining;
    u8 *data = NULL;
    ssize_t count = 0;
    int ret;

    TPD_INFO("<I> Read firmware register \n");

    if (argc != 2) {
        return scnprintf(buf, PAGE_SIZE,
            "Invalid num args %d\n"
            "  1. echo addr size > read_reg\n"
            "  2. cat read_reg\n", argc);
    }

    ret = kstrtou16(argv[0], 0, &addr);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid address: %s\n", argv[0]);
    }
    ret = kstrtou16(argv[1], 0, &size);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid size: %s\n", argv[1]);
    }

    data = (u8 *)kmalloc(size, GFP_KERNEL);
    if (data == NULL) {
        return scnprintf(buf, PAGE_SIZE, "Allocate buffer for read data failed\n");
    }

    TPD_INFO("<I> Read firmware register from 0x%04x size %u\n", addr, size);
    cts_lock_device(cts_dev);
    ret = cts_fw_reg_readsb(cts_dev, addr, data, (size_t)size);
    cts_unlock_device(cts_dev);
    if (ret) {
        count = scnprintf(buf, PAGE_SIZE,
            "Read firmware register from 0x%04x size %u failed %d\n",
            addr, size, ret);
        goto err_free_data;
    }

    remaining = size;
    for (i = 0; i < size && count <= PAGE_SIZE; i += PRINT_ROW_SIZE) {
        size_t linelen = min((size_t)remaining, (size_t)PRINT_ROW_SIZE);
        remaining -= PRINT_ROW_SIZE;

        count += scnprintf(buf + count, PAGE_SIZE - count, "%04x: ", addr);

        /* Lower version kernel return void */
        hex_dump_to_buffer(data + i, linelen, PRINT_ROW_SIZE, 1,
                    buf + count, PAGE_SIZE - count, true);
        count += strlen(buf + count);

        if (count < PAGE_SIZE) {
            buf[count++] = '\n';
            addr += PRINT_ROW_SIZE;
        } else {
            break;
        }
    }

err_free_data:
    kfree(data);

    return count;
#undef PRINT_ROW_SIZE
}

/* echo addr size > read_reg */
static ssize_t read_firmware_register_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return (argc == 0 ? 0 : count);
}
static DEVICE_ATTR(read_reg, S_IWUSR | S_IRUSR,
    read_firmware_register_show, read_firmware_register_store);



static ssize_t read_hw_reg_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#define PRINT_ROW_SIZE          (16)
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u32 addr, size, i, remaining;
    u8 *data = NULL;
    ssize_t count = 0;
    int ret;

    TPD_INFO("<I> Read hw register\n");

    if (argc != 2) {
        return scnprintf(buf, PAGE_SIZE,
            "Invalid num args %d\n"
            "  1. echo addr size > read_hw_reg\n"
            "  2. cat read_hw_reg\n", argc);
    }

    ret = kstrtou32(argv[0], 0, &addr);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid address: %s\n", argv[0]);
    }
    ret = kstrtou32(argv[1], 0, &size);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid size: %s\n", argv[1]);
    }

    data = (u8 *)kmalloc(size, GFP_KERNEL);
    if (data == NULL) {
        return scnprintf(buf, PAGE_SIZE, "Allocate buffer for read data failed\n");
    }

    TPD_INFO("<I> Read hw register from 0x%08x size %u\n", addr, size);
    cts_lock_device(cts_dev);
    ret = cts_hw_reg_readsb(cts_dev, addr, data, size);
    cts_unlock_device(cts_dev);

    if (ret) {
        count = scnprintf(buf, PAGE_SIZE, "Read hw register failed %d", ret);
        goto err_free_data;
    }

    remaining = size;
    for (i = 0; i < size && count <= PAGE_SIZE; i += PRINT_ROW_SIZE) {
        size_t linelen = min((size_t)remaining, (size_t)PRINT_ROW_SIZE);
        remaining -= PRINT_ROW_SIZE;

        count += scnprintf(buf + count, PAGE_SIZE - count, "%04x-%04x: ",
                (u16)(addr >> 16), (u16)addr);

        /* Lower version kernel return void */
        hex_dump_to_buffer(data + i, linelen, PRINT_ROW_SIZE, 1,
                    buf + count, PAGE_SIZE - count, true);
        count += strlen(buf + count);

        if (count < PAGE_SIZE) {
            buf[count++] = '\n';
            addr += PRINT_ROW_SIZE;
        } else {
            break;
        }
    }

err_free_data:
    kfree(data);

    return count;
#undef PRINT_ROW_SIZE
}

/* echo addr size > read_reg */
static ssize_t read_hw_reg_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return (argc == 0 ? 0 : count);
}

static DEVICE_ATTR(read_hw_reg, S_IRUSR | S_IWUSR, read_hw_reg_show, read_hw_reg_store);

static ssize_t write_hw_reg_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u32 addr;
    int i, ret;
    u8 *data = NULL;

    parse_arg(buf, count);

    TPD_INFO("<I> Write hw register\n");

    if (argc < 2) {
        TPD_INFO("<E> Too few args %d\n", argc);
        return -EFAULT;
    }

    ret = kstrtou32(argv[0], 0, &addr);
    if (ret) {
        TPD_INFO("<E> Invalid address %s\n", argv[0]);
        return -EINVAL;
    }

    data = (u8 *)kmalloc(argc - 1, GFP_KERNEL);
    if (data == NULL) {
        TPD_INFO("<E> Allocate buffer for write data failed\n");
        return -ENOMEM;
    }

    for (i = 1; i < argc; i++) {
        ret = kstrtou8(argv[i], 0, data + i - 1);
        if (ret) {
            TPD_INFO("<E> Invalid value %s\n", argv[i]);
            goto free_data;
        }
    }

    TPD_INFO("<I> Write hw register from 0x%08x size %u\n", addr, argc - 1);

    cts_lock_device(cts_dev);
    ret = cts_hw_reg_writesb(cts_dev, addr, data, argc - 1);
    cts_unlock_device(cts_dev);

    if (ret) {
        TPD_INFO("<E> Write hw register failed %d\n", ret);
    }

free_data:
    kfree(data);

    return (ret < 0 ? ret : count);
}

static DEVICE_ATTR(write_hw_reg, S_IWUSR, NULL, write_hw_reg_store);


static ssize_t curr_firmware_version_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Current firmware version: %04x\n",
        cts_data->cts_dev.fwdata.version);
}
static DEVICE_ATTR(curr_version, S_IRUGO, curr_firmware_version_show, NULL);

static ssize_t curr_ddi_version_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Current ddi version: %02x\n",
        cts_data->cts_dev.fwdata.ddi_version);
}
static DEVICE_ATTR(curr_ddi_version, S_IRUGO, curr_ddi_version_show, NULL);

static ssize_t rows_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Num rows: %u\n",
        cts_data->cts_dev.fwdata.rows);
}
static DEVICE_ATTR(rows, S_IRUGO, rows_show, NULL);

static ssize_t cols_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Num cols: %u\n",
        cts_data->cts_dev.fwdata.cols);
}
static DEVICE_ATTR(cols, S_IRUGO, cols_show, NULL);

static ssize_t res_x_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "X Resolution: %u\n",
        cts_data->cts_dev.fwdata.res_x);
}
static DEVICE_ATTR(res_x, S_IRUGO, res_x_show, NULL);

static ssize_t res_y_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Y Resolution: %u\n",
        cts_data->cts_dev.fwdata.res_y);
}
static DEVICE_ATTR(res_y, S_IRUGO, res_y_show, NULL);

static ssize_t monitor_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    int ret;
    u8  value;

    cts_lock_device(cts_dev);
    ret = cts_fw_reg_readb(&cts_data->cts_dev,
        CTS_DEVICE_FW_REG_FLAG_BITS, &value);
    cts_unlock_device(cts_dev);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Read firmware monitor enable register failed %d\n", ret);
    }

    return scnprintf(buf, PAGE_SIZE, "Monitor mode: %s\n",
        value & BIT(0) ? "Enable" : "Disable");
}

static ssize_t monitor_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    int ret;
    u8  value, enable = 0;

    if (buf[0] == 'Y' || buf[0] == 'y' || buf[0] == '1') {
        enable = 1;
    }

    TPD_INFO("<I> Write firmware monitor mode to '%c', %s\n",
        buf[0], enable ? "Enable" : "Disable");

    cts_lock_device(cts_dev);
    ret = cts_fw_reg_readb(&cts_data->cts_dev,
        CTS_DEVICE_FW_REG_FLAG_BITS, &value);
    cts_unlock_device(cts_dev);
    if (ret) {
        TPD_INFO("<E> Write firmware monitor enable register failed %d\n", ret);
        return -EIO;
    }

    if ((value & BIT(0)) && enable) {
        TPD_INFO("<I> Monitor mode already enabled\n");
    } else if ((value & BIT(0)) == 0 && enable == 0) {
        TPD_INFO("<I> Monitor mode already disabled\n");
    } else {
        if (enable) {
            value |= BIT(0);
        } else {
            value &= ~BIT(0);
        }

        cts_lock_device(cts_dev);
        ret = cts_fw_reg_writeb(&cts_data->cts_dev,
            CTS_DEVICE_FW_REG_FLAG_BITS, value);
        cts_unlock_device(cts_dev);
        if (ret) {
            TPD_INFO("<E> Write firmware monitor enable register failed %d\n", ret);
            return -EIO;
        }
    }

    return count;
}
static DEVICE_ATTR(monitor_mode, S_IRUGO, monitor_mode_show, monitor_mode_store);

static ssize_t auto_compensate_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    int ret;
    u8  value;

    cts_lock_device(cts_dev);
    ret = cts_fw_reg_readb(&cts_data->cts_dev,
        CTS_DEVICE_FW_REG_AUTO_CALIB_COMP_CAP_ENABLE, &value);
    cts_unlock_device(cts_dev);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Read auto compensate enable register failed %d\n", ret);
    }

    return scnprintf(buf, PAGE_SIZE,
        "Auto compensate: %s\n", value ? "Enable" : "Disable");
}
static DEVICE_ATTR(auto_compensate, S_IRUGO, auto_compensate_show, NULL);

static ssize_t updating_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Updating: %s\n",
        cts_data->cts_dev.rtdata.updating ? "Y" : "N");
}
static DEVICE_ATTR(updating, S_IRUGO, updating_show, NULL);

static struct attribute *cts_dev_firmware_atts[] = {
    &dev_attr_curr_version.attr,
    &dev_attr_curr_ddi_version.attr,
    &dev_attr_rows.attr,
    &dev_attr_cols.attr,
    &dev_attr_res_x.attr,
    &dev_attr_res_y.attr,
    &dev_attr_monitor_mode.attr,
    &dev_attr_auto_compensate.attr,
    &dev_attr_updating.attr,
    NULL
};

static const struct attribute_group cts_dev_firmware_attr_group = {
    .name  = "cts_firmware",
    .attrs = cts_dev_firmware_atts,
};

static ssize_t flash_info_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    const struct cts_flash *flash;

    if (cts_dev->flash == NULL) {
        bool program_mode;
        bool enabled;
        int  ret;

        program_mode = cts_is_device_program_mode(cts_dev);
        enabled = cts_is_device_enabled(cts_dev);

        ret = cts_prepare_flash_operation(cts_dev);
        if (ret) {
            return scnprintf(buf, PAGE_SIZE, "Prepare flash operation failed %d", ret);
        }

        cts_post_flash_operation(cts_dev);

        if (!program_mode) {
            ret = cts_enter_normal_mode(cts_dev);
            if (ret) {
                return scnprintf(buf, PAGE_SIZE, "Enter normal mode failed %d", ret);
            }
        }

        if (enabled) {
            ret = cts_start_device(cts_dev);
            if (ret) {
                return scnprintf(buf, PAGE_SIZE, "Start device failed %d", ret);
            }
        }

        if (cts_dev->flash == NULL) {
            return scnprintf(buf, PAGE_SIZE, "Flash not found\n");
        }
    }

    flash = cts_dev->flash;
    return scnprintf(buf, PAGE_SIZE,
                        "%s:\n"
                        "  JEDEC ID   : %06X\n"
                        "  Page size  : 0x%zx\n"
                        "  Sector size: 0x%zx\n"
                        "  Block size : 0x%zx\n"
                        "  Total size : 0x%zx\n",
        flash->name, flash->jedec_id, flash->page_size,
        flash->sector_size, flash->block_size, flash->total_size);
}
static DEVICE_ATTR(info, S_IRUGO, flash_info_show, NULL);

static ssize_t read_flash_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u32 flash_addr, size, i, remaining;
    u8 *data = NULL;
    ssize_t count = 0;
    int ret;
    bool program_mode;
    bool enabled;
    loff_t pos = 0;

    if (argc != 2 && argc != 3) {
        return scnprintf(buf, PAGE_SIZE, "Invalid num args %d\n", argc);
    }

    ret = kstrtou32(argv[0], 0, &flash_addr);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid flash addr: %s\n", argv[0]);
    }
    ret = kstrtou32(argv[1], 0, &size);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid size: %s\n", argv[1]);
    }

    data = (u8 *)kmalloc(size, GFP_KERNEL);
    if (data == NULL) {
        return scnprintf(buf, PAGE_SIZE, "Allocate buffer for read data failed\n");
    }

    TPD_INFO("<I> Read flash from 0x%06x size %u%s%s\n",
        flash_addr, size, argc == 3 ? " to file " : "",
        argc == 3 ? argv[2] : "");

    cts_lock_device(cts_dev);
    program_mode = cts_is_device_program_mode(cts_dev);
    enabled = cts_is_device_enabled(cts_dev);

    ret = cts_prepare_flash_operation(cts_dev);
    if (ret) {
        count += scnprintf(buf, PAGE_SIZE, "Prepare flash operation failed %d", ret);
        goto err_free_data;
    }

    ret = cts_read_flash(cts_dev, flash_addr, data, size);
    if (ret) {
        count = scnprintf(buf, PAGE_SIZE, "Read flash data failed %d\n", ret);
        goto err_post_flash_operation;
    }

    if (argc == 3) {
        struct file *file;

        TPD_INFO("<I> Write flash data to file '%s'\n", argv[2]);

        file = filp_open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (IS_ERR(file)) {
            count += scnprintf(buf, PAGE_SIZE, "Open file '%s' failed %ld",
                argv[2], PTR_ERR(file));
            goto err_post_flash_operation;
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
        ret = kernel_write(file, data, size, &pos);
#else
        ret = kernel_write(file, data, size, pos);
#endif
        if (ret != size) {
            count += scnprintf(buf, PAGE_SIZE, "Write flash data to file '%s' failed %d",
                argv[2], ret);
        }

        ret = filp_close(file, NULL);
        if (ret) {
            count += scnprintf(buf, PAGE_SIZE, "Close file '%s' failed %d", argv[2], ret);
        }
    } else {
#define PRINT_ROW_SIZE          (16)
        remaining = size;
        for (i = 0; i < size && count <= PAGE_SIZE; i += PRINT_ROW_SIZE) {
            size_t linelen = min((size_t)remaining, (size_t)PRINT_ROW_SIZE);
            remaining -= PRINT_ROW_SIZE;

            count += scnprintf(buf + count, PAGE_SIZE - count - 1,
                        "%04x-%04x: ", flash_addr >> 16, flash_addr & 0xFFFF);
            /* Lower version kernel return void */
            hex_dump_to_buffer(data + i, linelen, PRINT_ROW_SIZE, 1,
                        buf + count, PAGE_SIZE - count - 1, true);
            count += strlen(buf + count);
            buf[count++] = '\n';
            flash_addr += linelen;
#undef PRINT_ROW_SIZE
        }
    }

err_post_flash_operation:
    cts_post_flash_operation(cts_dev);

    if (!program_mode) {
        int r = cts_enter_normal_mode(cts_dev);
        if (r) {
            count += scnprintf(buf, PAGE_SIZE, "Enter normal mode failed %d", r);
        }
    }

    if (enabled) {
        int r = cts_start_device(cts_dev);
        if (r) {
               cts_unlock_device(cts_dev);
            return scnprintf(buf, PAGE_SIZE, "Start device failed %d", r);
        }
    }
err_free_data:
    cts_unlock_device(cts_dev);
    kfree(data);

    return (ret < 0 ? ret : count);
}

/* echo start_addr size [filepath] > read */
static ssize_t read_flash_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return count;
}
static DEVICE_ATTR(read, S_IWUSR | S_IRUGO, read_flash_show, read_flash_store);

/* echo addr size > erase */
static ssize_t erase_flash_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u32 flash_addr, size;
    int ret;
    bool program_mode;
    bool enabled;

    parse_arg(buf, count);

    if (argc != 2) {
        TPD_INFO("<E> Invalid num args %d\n", argc);
        return -EFAULT;
    }

    ret = kstrtou32(argv[0], 0, &flash_addr);
    if (ret) {
        TPD_INFO("<E> Invalid flash addr: %s\n", argv[0]);
        return -EINVAL;
    }
    ret = kstrtou32(argv[1], 0, &size);
    if (ret) {
        TPD_INFO("<E> Invalid size: %s\n", argv[1]);
        return -EINVAL;
    }

    TPD_INFO("<I> Erase flash from 0x%06x size %u\n", flash_addr, size);

    cts_lock_device(cts_dev);
    program_mode = cts_is_device_program_mode(cts_dev);
    enabled = cts_is_device_enabled(cts_dev);

    ret = cts_prepare_flash_operation(cts_dev);
    if (ret) {
        TPD_INFO("<E> Prepare flash operation failed %d\n", ret);
        cts_unlock_device(cts_dev);
        return ret;
    }

    ret = cts_erase_flash(cts_dev, flash_addr, size);
    if (ret) {
        TPD_INFO("<E> Erase flash from 0x%06x size %u failed %d\n",
            flash_addr, size, ret);
        goto err_post_flash_operation;
    }

err_post_flash_operation:
    cts_post_flash_operation(cts_dev);

    if (!program_mode) {
        int r = cts_enter_normal_mode(cts_dev);
        if (r) {
            TPD_INFO("<E> Enter normal mode failed %d\n", r);
        }
    }

    if (enabled) {
        int r = cts_start_device(cts_dev);
        if (r) {
            TPD_INFO("<E> Start device failed %d\n", r);
        }
    }
    cts_unlock_device(cts_dev);

    return (ret < 0 ? ret : count);
}
static DEVICE_ATTR(erase, S_IWUSR, NULL, erase_flash_store);

static struct attribute *cts_dev_flash_attrs[] = {
    &dev_attr_info.attr,
    &dev_attr_read.attr,
    &dev_attr_erase.attr,

    NULL
};

static const struct attribute_group cts_dev_flash_attr_group = {
    .name  = "flash",
    .attrs = cts_dev_flash_attrs,
};

static ssize_t open_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_test_param test_param = {
        .test_item = CTS_TEST_OPEN,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MIN |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath =
            "/sdcard/chipone-tddi/test/open-test-data.txt",
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
    };
    int min = 0;
    int ret;
    ktime_t start_time, end_time, delta_time;

    if (argc != 1) {
        return scnprintf(buf, PAGE_SIZE, "Invalid num args %d\n", argc);
    }

    ret = kstrtoint(argv[0], 0, &min);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid min thres: %s\n", argv[0]);
    }

    TPD_INFO("<I> Open test, threshold = %u\n", min);

    test_param.min = &min;

    start_time = ktime_get();

    ret = cts_test_open(cts_dev, &test_param);

    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);

    if (ret > 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Open test has %d nodes FAIL, min: %u, ELAPSED TIME: %lldms\n",
            ret, min, ktime_to_ms(delta_time));
    } else if (ret < 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Open test FAIL %d(%s), ELAPSED TIME: %lldms\n",
            ret, cts_strerror(ret), ktime_to_ms(delta_time));
    } else {
        return scnprintf(buf, PAGE_SIZE,
            "Open test PASS, ELAPSED TIME: %lldms\n",
            ktime_to_ms(delta_time));
    }
}

/* echo threshold > open_test */
static ssize_t open_test_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return count;
}
static DEVICE_ATTR(open_test, S_IWUSR | S_IRUGO, open_test_show, open_test_store);

static ssize_t short_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_test_param test_param = {
        .test_item = CTS_TEST_SHORT,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MIN |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath =
            "/sdcard/chipone-tddi/test/short-test-data.txt",
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
    };
    int min = 0;
    int ret;
    ktime_t start_time, end_time, delta_time;

    if (argc != 1) {
        return scnprintf(buf, PAGE_SIZE, "Invalid num args %d\n", argc);
    }

    ret = kstrtoint(argv[0], 0, &min);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE, "Invalid min thres: %s\n", argv[0]);
    }

    TPD_INFO("<I> Short test, threshold = %u\n", min);

    test_param.min = &min;

    start_time = ktime_get();

    ret = cts_test_short(cts_dev, &test_param);

    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);

    if (ret > 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Short test has %d nodes FAIL, min: %u, ELAPSED TIME: %lldms\n",
            ret, min, ktime_to_ms(delta_time));
    } else if (ret < 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Short test FAIL %d(%s), ELAPSED TIME: %lldms\n",
            ret, cts_strerror(ret), ktime_to_ms(delta_time));
    } else {
        return scnprintf(buf, PAGE_SIZE,
            "Short test PASS, ELAPSED TIME: %lldms\n",
            ktime_to_ms(delta_time));
    }
}

/* echo threshod > short_test */
static ssize_t short_test_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return count;
}
static DEVICE_ATTR(short_test, S_IWUSR | S_IRUGO,
        short_test_show, short_test_store);

static ssize_t testing_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Testting: %s\n",
        cts_data->cts_dev.rtdata.testing ? "Y" : "N");
}
static DEVICE_ATTR(testing, S_IRUGO, testing_show, NULL);

static ssize_t reset_pin_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#ifdef CFG_CTS_HAS_RESET_PIN
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_test_param test_param = {
        .test_item = CTS_TEST_RESET_PIN,
        .flags = 0,
    };
    int ret;
    ktime_t start_time, end_time, delta_time;

    start_time = ktime_get();

    ret = cts_test_reset_pin(cts_dev, &test_param);

    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);

    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Reset-Pin test FAIL %d(%s), ELAPSED TIME: %lldms\n",
                ret, cts_strerror(ret), ktime_to_ms(delta_time));
    } else {
        return scnprintf(buf, PAGE_SIZE,
            "Reset-Pin test PASS, ELAPSED TIME: %lldms\n",
                ktime_to_ms(delta_time));
    }
#else /* CFG_CTS_HAS_RESET_PIN */
    return scnprintf(buf, PAGE_SIZE,
        "Reset-Pin test NOT supported(CFG_CTS_HAS_RESET_PIN not defined)\n");
#endif
}
static DEVICE_ATTR(reset_pin_test, S_IRUGO, reset_pin_test_show, NULL);

static ssize_t int_pin_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_test_param test_param = {
        .test_item = CTS_TEST_INT_PIN,
        .flags = 0,
    };
    int ret;
    ktime_t start_time, end_time, delta_time;

    start_time = ktime_get();

    ret = cts_test_int_pin(cts_dev, &test_param);

    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);

    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Int-Pin test FAIL %d(%s), ELAPSED TIME: %lldms\n",
                ret, cts_strerror(ret), ktime_to_ms(delta_time));
    } else {
        return scnprintf(buf, PAGE_SIZE,
            "Int-Pin test PASS, ELAPSED TIME: %lldms\n",
                ktime_to_ms(delta_time));
    }
}
static DEVICE_ATTR(int_pin_test, S_IRUGO, int_pin_test_show, NULL);

static ssize_t compensate_cap_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_test_param test_param = {
        .test_item = CTS_TEST_COMPENSATE_CAP,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MIN |
                 CTS_TEST_FLAG_VALIDATE_MAX |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath =
            "/sdcard/chipone-tddi/test/comp-cap-test-data.txt",
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
    };
    int min = 0, max = 0;
    int ret;
    ktime_t start_time, end_time, delta_time;

    if (argc != 2) {
        return scnprintf(buf, PAGE_SIZE, "Invalid num args\n"
                   "USAGE:\n"
                   "  1. echo min max > compensate_cap_test\n"
                   "  2. cat compensate_cap_test\n");
    }

    ret = kstrtoint(argv[0], 0, &min);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Invalid min thres: %s\n", argv[0]);
    }

    ret = kstrtoint(argv[1], 0, &max);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Invalid max thres: %s\n", argv[1]);
    }

    TPD_INFO("<I> Compensate cap test, min: %u, max: %u\n",
         min, max);

    test_param.min = &min;
    test_param.max = &max;

    start_time = ktime_get();

    ret = cts_test_compensate_cap(cts_dev, &test_param);

    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);

    if (ret > 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Compensate cap test has %d nodes FAIL, "
            "threshold[%u, %u], ELAPSED TIME: %lldms\n",
            ret, min, max, ktime_to_ms(delta_time));
    } else if (ret < 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Compensate cap test FAIL %d(%s), ELAPSED TIME: %lldms\n",
            ret, cts_strerror(ret), ktime_to_ms(delta_time));
    } else {
        return scnprintf(buf, PAGE_SIZE,
            "Compensate cap test PASS, ELAPSED TIME: %lldms\n",
            ktime_to_ms(delta_time));
    }
}

static ssize_t compensate_cap_test_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return count;
}
static DEVICE_ATTR(compensate_cap_test, S_IWUSR | S_IRUGO,
        compensate_cap_test_show, compensate_cap_test_store);

static ssize_t rawdata_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_rawdata_test_priv_param priv_param = {
        .frames = 16,
        //.work_mode = 0,
    };
    struct cts_test_param test_param = {
        .test_item = CTS_TEST_RAWDATA,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MIN |
                 CTS_TEST_FLAG_VALIDATE_MAX |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath =
            "/sdcard/chipone-tddi/test/rawdata-test-data.txt",
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
        .priv_param = &priv_param,
        .priv_param_size = sizeof(priv_param),
    };

    int min, max;
    int ret;
    ktime_t start_time, end_time, delta_time;

    if (argc < 2 || argc > 3) {
        return scnprintf(buf, PAGE_SIZE, "Invalid num args\n"
            "USAGE:\n"
            "  1. echo min max [frames] > rawdata_test\n"
            "  2. cat rawdata_test\n");
    }

    ret = kstrtoint(argv[0], 0, &min);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Invalid min thres: %s\n", argv[0]);
    }

    ret = kstrtoint(argv[1], 0, &max);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Invalid max thres: %s\n", argv[1]);
    }

    if (argc > 2) {
        ret = kstrtou32(argv[2], 0, &priv_param.frames);
        if (ret) {
            return scnprintf(buf, PAGE_SIZE,
                "Invalid frames: %s\n", argv[2]);
        }
    }
    TPD_INFO("<I> Rawdata test, frames: %u min: %d, max: %d\n",
        priv_param.frames, min, max);

    test_param.min = &min;
    test_param.max = &max;

    start_time = ktime_get();

    ret = cts_test_rawdata(cts_dev, &test_param);

    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);

    if (ret > 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Rawdata test has %d nodes FAIL, threshold[%u, %u], "
            "ELAPSED TIME: %lldms\n",
            ret, min, max, ktime_to_ms(delta_time));
    } else if (ret < 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Rawdata test FAIL %d(%s), ELAPSED TIME: %lldms\n",
            ret, cts_strerror(ret), ktime_to_ms(delta_time));
    } else {
        return scnprintf(buf, PAGE_SIZE,
            "Rawdata test PASS, ELAPSED TIME: %lldms\n",
            ktime_to_ms(delta_time));
    }
}

static ssize_t rawdata_test_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return count;
}
static DEVICE_ATTR(rawdata_test, S_IWUSR | S_IRUGO,
        rawdata_test_show, rawdata_test_store);

static ssize_t noise_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_noise_test_priv_param priv_param = {
        .frames = 50,
        //.work_mode = 0,
    };
    struct cts_test_param test_param = {
        .test_item = CTS_TEST_NOISE,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MAX |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath =
            "/sdcard/chipone-tddi/test/noise-test-data.txt",
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
        .priv_param = &priv_param,
        .priv_param_size = sizeof(priv_param),
    };

    int max;
    int ret;
    ktime_t start_time, end_time, delta_time;

    if (argc < 1 || argc > 2) {
        return scnprintf(buf, PAGE_SIZE, "Invalid num args\n"
            "USAGE:\n"
            "  1. echo threshold [frames] > noise_test\n"
            "  2. cat noise_test\n");
    }

    ret = kstrtoint(argv[0], 0, &max);
    if (ret) {
        return scnprintf(buf, PAGE_SIZE,
            "Invalid max thres: %s\n", argv[0]);
    }

    if (argc > 1) {
        ret = kstrtou32(argv[1], 0, &priv_param.frames);
        if (ret) {
            return scnprintf(buf, PAGE_SIZE,
                "Invalid frames: %s\n", argv[1]);
        }
    }
    TPD_INFO("<I> Noise test, frames: %u threshold: %d\n",
        priv_param.frames, max);

    test_param.max = &max;

    start_time = ktime_get();

    ret = cts_test_noise(cts_dev, &test_param);

    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);

    if (ret > 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Noise test has %d nodes FAIL, max: %u, ELAPSED TIME: %lldms\n",
            ret, max, ktime_to_ms(delta_time));
    } else if (ret < 0) {
        return scnprintf(buf, PAGE_SIZE,
            "Noise test FAIL %d(%s), ELAPSED TIME: %lldms\n",
            ret, cts_strerror(ret), ktime_to_ms(delta_time));
    } else {
        return scnprintf(buf, PAGE_SIZE,
            "Noise test PASS, ELAPSED TIME: %lldms\n",
            ktime_to_ms(delta_time));
    }
}

static ssize_t noise_test_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    parse_arg(buf, count);

    return count;
}
static DEVICE_ATTR(noise_test, S_IWUSR | S_IRUGO,
        noise_test_show, noise_test_store);

static ssize_t factory_test_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#define TOUCH_DATA_DIRECTORY_PREFIX     "/sdcard/chipone-tddi/test/"
#define RAWDATA_TEST_DATA_FILENAME      "rawdata-test-data.txt"
#define NOISE_TEST_DATA_FILENAME        "noise-test-data.txt"
#define OPEN_TEST_DATA_FILENAME         "open-test-data.txt"
#define SHORT_TEST_DATA_FILENAME        "short-test-data.txt"
#define COMP_CAP_TEST_DATA_FILENAME     "comp-cap-test-data.txt"

    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    struct cts_test_param reset_pin_test_param = {
        .test_item = CTS_TEST_RESET_PIN,
        .flags = 0,
    };
    struct cts_test_param int_pin_test_param = {
        .test_item = CTS_TEST_INT_PIN,
        .flags = 0,
    };
    struct cts_rawdata_test_priv_param rawdata_test_priv_param = {
        .frames = 16,
        //.work_mode = 0,
    };
    struct cts_test_param rawdata_test_param = {
        .test_item = CTS_TEST_RAWDATA,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MIN |
                 CTS_TEST_FLAG_VALIDATE_MAX |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath = NULL,
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
        .priv_param = &rawdata_test_priv_param,
        .priv_param_size = sizeof(rawdata_test_priv_param),
    };
    struct cts_noise_test_priv_param noise_test_priv_param = {
        .frames = 50,
        //.work_mode = 0,
    };
    struct cts_test_param noise_test_param = {
        .test_item = CTS_TEST_NOISE,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MAX |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath = NULL,
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
        .priv_param = &noise_test_priv_param,
        .priv_param_size = sizeof(noise_test_priv_param),
    };
    struct cts_test_param open_test_param = {
        .test_item = CTS_TEST_OPEN,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MIN |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath = NULL,
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
    };
    struct cts_test_param short_test_param = {
        .test_item = CTS_TEST_SHORT,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MIN |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath = NULL,
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
    };
    struct cts_test_param comp_cap_test_param = {
        .test_item = CTS_TEST_COMPENSATE_CAP,
        .flags = CTS_TEST_FLAG_VALIDATE_DATA |
                 CTS_TEST_FLAG_VALIDATE_MIN |
                 CTS_TEST_FLAG_VALIDATE_MAX |
                 CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE |
                 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE,
        .test_data_filepath = NULL,
        .num_invalid_node = 0,
        .invalid_nodes = NULL,
    };
    int rawdata_min;
    int rawdata_max;
    int noise_max = 50;
    int open_min = 200;
    int short_min = 200;
    int comp_cap_min = 20;
    int comp_cap_max = 100;
    int rawdata_test_result = 0;
    int reset_pin_test_result = 0;
    int int_pin_test_result = 0;
    int noise_test_result = 0;
    int open_test_result = 0;
    int short_test_result = 0;
    int comp_cap_test_result = 0;
    int count = 0;
    struct timeval  ktv;
    struct rtc_time rtc_tm;
    char touch_data_filepath[256];
    ktime_t start_time, end_time, delta_time;

    rawdata_min = cts_dev->fwdata.rawdata_target * 3 / 4;
    rawdata_max = cts_dev->fwdata.rawdata_target * 5 / 4;
    rawdata_test_param.min = &rawdata_min;
    rawdata_test_param.max = &rawdata_max;
    noise_test_param.max = &noise_max;
    open_test_param.min = &open_min;
    short_test_param.min = &short_min;
    comp_cap_test_param.min = &comp_cap_min;
    comp_cap_test_param.max = &comp_cap_max;

    do_gettimeofday(&ktv);
    ktv.tv_sec -= sys_tz.tz_minuteswest * 60;
    rtc_time_to_tm(ktv.tv_sec, &rtc_tm);

    snprintf(touch_data_filepath, sizeof(touch_data_filepath),
        TOUCH_DATA_DIRECTORY_PREFIX"touch_data_%04d%02d%02d_%02d%02d%02d/"
        RAWDATA_TEST_DATA_FILENAME,
        rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
        rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
    rawdata_test_param.test_data_filepath =
        kstrdup(touch_data_filepath, GFP_KERNEL);
    snprintf(touch_data_filepath, sizeof(touch_data_filepath),
        TOUCH_DATA_DIRECTORY_PREFIX"touch_data_%04d%02d%02d_%02d%02d%02d/"
        NOISE_TEST_DATA_FILENAME,
        rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
        rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
    noise_test_param.test_data_filepath =
        kstrdup(touch_data_filepath, GFP_KERNEL);
    snprintf(touch_data_filepath, sizeof(touch_data_filepath),
        TOUCH_DATA_DIRECTORY_PREFIX"touch_data_%04d%02d%02d_%02d%02d%02d/"
        OPEN_TEST_DATA_FILENAME,
        rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
        rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
    open_test_param.test_data_filepath =
        kstrdup(touch_data_filepath, GFP_KERNEL);
    snprintf(touch_data_filepath, sizeof(touch_data_filepath),
        TOUCH_DATA_DIRECTORY_PREFIX"touch_data_%04d%02d%02d_%02d%02d%02d/"
        SHORT_TEST_DATA_FILENAME,
        rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
        rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
    short_test_param.test_data_filepath =
        kstrdup(touch_data_filepath, GFP_KERNEL);
    snprintf(touch_data_filepath, sizeof(touch_data_filepath),
        TOUCH_DATA_DIRECTORY_PREFIX"touch_data_%04d%02d%02d_%02d%02d%02d/"
        COMP_CAP_TEST_DATA_FILENAME,
        rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
        rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);
    comp_cap_test_param.test_data_filepath =
        kstrdup(touch_data_filepath, GFP_KERNEL);

    TPD_INFO("<I> Factory test: rawdata: [%u, %u] noise: %u, "
             "open: %u, short: %u, comp_cap: [%u, %u], "
             "touch data dir: "
                TOUCH_DATA_DIRECTORY_PREFIX"%04d%02d%02d_%02d%02d%02d/\n",
        rawdata_min, rawdata_max, noise_max, open_min, short_min,
        comp_cap_min, comp_cap_max,
        rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
        rtc_tm.tm_hour, rtc_tm.tm_min,rtc_tm.tm_sec);

    start_time = ktime_get();

    reset_pin_test_result =
        cts_test_reset_pin(cts_dev, &reset_pin_test_param);
    int_pin_test_result =
        cts_test_int_pin(cts_dev, &int_pin_test_param);
    rawdata_test_result =
        cts_test_rawdata(cts_dev, &rawdata_test_param);
    noise_test_result =
        cts_test_noise(cts_dev, &noise_test_param);
    open_test_result =
        cts_test_open(cts_dev, &open_test_param);
    short_test_result =
        cts_test_short(cts_dev, &short_test_param);
    comp_cap_test_result =
        cts_test_compensate_cap(cts_dev, &comp_cap_test_param);

    if (rawdata_test_param.test_data_filepath) {
        kfree(rawdata_test_param.test_data_filepath);
    }
    if (noise_test_param.test_data_filepath) {
        kfree(noise_test_param.test_data_filepath);
    }
    if (open_test_param.test_data_filepath) {
        kfree(open_test_param.test_data_filepath);
    }
    if (short_test_param.test_data_filepath) {
        kfree(short_test_param.test_data_filepath);
    }
    if (comp_cap_test_param.test_data_filepath) {
        kfree(comp_cap_test_param.test_data_filepath);
    }

    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);
    count += scnprintf(buf + count, PAGE_SIZE,
        "Factory test, ELAPSED TIME: %lldms\n",
        ktime_to_ms(delta_time));

    if (reset_pin_test_result) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d(%s)\n", "Reset-Pin",
            reset_pin_test_result, cts_strerror(reset_pin_test_result));
    } else {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: PASS\n", "Reset-Pin");
    }
    if (int_pin_test_result) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d(%s)\n", "Int-Pin",
            int_pin_test_result, cts_strerror(int_pin_test_result));
    } else {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: PASS\n", "Int-Pin");
    }
    if (rawdata_test_result > 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d nodes\n", "Rawdata",
            rawdata_test_result);
    } else if (rawdata_test_result < 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d(%s)\n", "Rawdata",
            rawdata_test_result, cts_strerror(rawdata_test_result));
    } else {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: PASS\n", "Rawdata");
    }
    if (noise_test_result > 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d nodes\n", "Noise",
            noise_test_result);
    } else if (noise_test_result < 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d(%s)\n", "Noise",
            noise_test_result, cts_strerror(noise_test_result));
    } else {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: PASS\n",  "Noise");
    }
    if (open_test_result > 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d nodes\n", "Open",
            open_test_result);
    } else if (open_test_result < 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d(%s)\n", "Open",
            open_test_result, cts_strerror(open_test_result));
    } else {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: PASS\n", "Open");
    }
    if (short_test_result > 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d nodes\n", "Short",
            short_test_result);
    } else if (short_test_result < 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d(%s)\n", "Short",
            short_test_result, cts_strerror(short_test_result));
    } else {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: PASS\n", "Short");
    }
    if (comp_cap_test_result > 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d nodes\n", "Comp-Cap",
            comp_cap_test_result);
    } else if (comp_cap_test_result < 0) {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: FAIL %d(%s)\n", "Comp-Cap",
            comp_cap_test_result, cts_strerror(comp_cap_test_result));
    } else {
        count += scnprintf(buf + count, PAGE_SIZE,
            "%-10s test: PASS\n", "Comp-Cap");
    }

    return count;

#undef TOUCH_DATA_DIRECTORY_PREFIX
#undef RAWDATA_TEST_DATA_FILENAME
#undef NOISE_TEST_DATA_FILENAME
#undef OPEN_TEST_DATA_FILENAME
#undef SHORT_TEST_DATA_FILENAME
#undef COMP_CAP_TEST_DATA_FILENAME
}
static DEVICE_ATTR(factory_test,S_IRUGO,
        factory_test_show, NULL);

static struct attribute *cts_dev_test_atts[] = {
    &dev_attr_testing.attr,
    &dev_attr_reset_pin_test.attr,
    &dev_attr_int_pin_test.attr,
    &dev_attr_rawdata_test.attr,
    &dev_attr_noise_test.attr,
    &dev_attr_open_test.attr,
    &dev_attr_short_test.attr,
    &dev_attr_compensate_cap_test.attr,
    &dev_attr_factory_test.attr,
    NULL
};

static const struct attribute_group cts_dev_test_attr_group = {
    .name  = "test",
    .attrs = cts_dev_test_atts,
};

static ssize_t ic_type_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "IC Type : %s\n",
        cts_data->cts_dev.hwdata->name);
}
static DEVICE_ATTR(ic_type, S_IRUGO, ic_type_show, NULL);

static ssize_t program_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    return scnprintf(buf, PAGE_SIZE, "Program mode: %s\n",
        cts_data->cts_dev.rtdata.program_mode ? "Y" : "N");
}
static ssize_t program_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    int ret;
    parse_arg(buf, count);

    if (argc != 1) {
        TPD_INFO("<E> Invalid num args %d\n", argc);
        return -EFAULT;
    }

    if (*argv[0] == '1' || tolower(*argv[0]) == 'y') {
        ret = cts_enter_program_mode(&cts_data->cts_dev);
        if (ret) {
            TPD_INFO("<E> Enter program mode failed %d\n", ret);
            return ret;
        }
    } else if (*argv[0] == '0' || tolower(*argv[0]) == 'n') {
        ret = cts_enter_normal_mode(&cts_data->cts_dev);
        if (ret) {
            TPD_INFO("<E> Exit program mode failed %d\n", ret);
            return ret;
        }
    } else {
        TPD_INFO("<E> Invalid args\n");
    }

    return count;
}
static DEVICE_ATTR(program_mode, S_IWUSR | S_IRUGO,
        program_mode_show, program_mode_store);

static ssize_t rawdata_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#define RAWDATA_BUFFER_SIZE(cts_dev) \
    (cts_dev->fwdata.rows * cts_dev->fwdata.cols * 2)

    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u16 *rawdata = NULL;
    int ret, r, c, count = 0;
    u32 max, min, sum, average;
    int max_r, max_c, min_r, min_c;
    bool data_valid = true;

    TPD_INFO("<I> Show rawdata\n");

    rawdata = (u16 *)kmalloc(RAWDATA_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (rawdata == NULL) {
        return scnprintf(buf, PAGE_SIZE, "Allocate memory for rawdata failed\n");
    }

    cts_lock_device(cts_dev);
    ret = cts_enable_get_rawdata(cts_dev);
    if (ret) {
        count += scnprintf(buf, PAGE_SIZE, "Enable read raw data failed %d\n", ret);
        goto err_free_rawdata;
    }

    ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
    if (ret) {
        count += scnprintf(buf, PAGE_SIZE, "Send cmd QUIT_GESTURE_MONITOR failed %d\n", ret);
        goto err_free_rawdata;
    }
    msleep(50);

    ret = cts_get_rawdata(cts_dev, rawdata);
    if(ret) {
        count += scnprintf(buf, PAGE_SIZE, "Get raw data failed %d\n", ret);
        data_valid = false;
        // Fall through to disable get rawdata
    }
    ret = cts_disable_get_rawdata(cts_dev);
    if (ret) {
        count += scnprintf(buf, PAGE_SIZE, "Disable read raw data failed %d\n", ret);
        // Fall through to show rawdata
    }

    if (data_valid) {
        max = min = rawdata[0];
        sum = 0;
        max_r = max_c = min_r = min_c = 0;
        for (r = 0; r < cts_dev->fwdata.rows; r++) {
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                u16 val = rawdata[r * cts_dev->fwdata.cols + c];
                sum += val;
                if (val > max) {
                    max = val;
                    max_r = r;
                    max_c = c;
                } else if (val < min) {
                    min = val;
                    min_r = r;
                    min_c = c;
                }
            }
        }
        average = sum / (cts_dev->fwdata.rows * cts_dev->fwdata.cols);

        count += scnprintf(buf + count, PAGE_SIZE - count,
            SPLIT_LINE_STR
            "Raw data MIN: [%d][%d]=%u, MAX: [%d][%d]=%u, AVG=%u\n"
            SPLIT_LINE_STR
            "   |  ", min_r, min_c, min, max_r, max_c, max, average);
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            count += scnprintf(buf + count, PAGE_SIZE - count, COL_NUM_FORMAT_STR, c);
        }
        count += scnprintf(buf + count, PAGE_SIZE - count, "\n" SPLIT_LINE_STR);

        for (r = 0; r < cts_dev->fwdata.rows && count < PAGE_SIZE; r++) {
            count += scnprintf(buf + count, PAGE_SIZE - count, ROW_NUM_FORMAT_STR, r);
            for (c = 0; c < cts_dev->fwdata.cols && count < PAGE_SIZE; c++) {
                count += scnprintf(buf + count, PAGE_SIZE - count,
                    DATA_FORMAT_STR, rawdata[r * cts_dev->fwdata.cols + c]);
            }
            buf[count++] = '\n';
        }
    }

err_free_rawdata:
    cts_unlock_device(cts_dev);
    kfree(rawdata);

    return (data_valid ? count : ret);

#undef RAWDATA_BUFFER_SIZE
}
static DEVICE_ATTR(rawdata, S_IRUGO, rawdata_show, NULL);

static ssize_t diffdata_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    s16 *diffdata = NULL;
    int ret, r, c, count = 0;
    int max, min, sum, average;
    int max_r, max_c, min_r, min_c;
    bool data_valid = true;

    TPD_INFO("<I> Show diffdata\n");

    diffdata = (s16 *)kmalloc(DIFFDATA_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (diffdata == NULL) {
        TPD_INFO("<E> Allocate memory for diffdata failed\n");
        return -ENOMEM;
    }

    cts_lock_device(cts_dev);
    ret = cts_enable_get_rawdata(cts_dev);
    if (ret) {
        TPD_INFO("<E> Enable read diff data failed %d\n", ret);
        goto err_free_diffdata;
    }

    ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
    if (ret) {
        TPD_INFO("<E> Send cmd QUIT_GESTURE_MONITOR failed %d\n", ret);
        goto err_free_diffdata;
    }
    msleep(50);

    ret = cts_get_diffdata(cts_dev, diffdata);
    if(ret) {
        TPD_INFO("<E> Get diff data failed %d\n", ret);
        data_valid = false;
        // Fall through to disable get diffdata
    }
    ret = cts_disable_get_rawdata(cts_dev);
    if (ret) {
        TPD_INFO("<E> Disable read diff data failed %d\n", ret);
        // Fall through to show diffdata
    }

    if (data_valid) {
        max = min = diffdata[0];
        sum = 0;
        max_r = max_c = min_r = min_c = 0;
        for (r = 0; r < cts_dev->fwdata.rows; r++) {
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                s16 val = diffdata[r * cts_dev->fwdata.cols + c];

                sum += val;
                if (val > max) {
                    max = val;
                    max_r = r;
                    max_c = c;
                } else if (val < min) {
                    min = val;
                    min_r = r;
                    min_c = c;
                }
            }
        }
        average = sum / (cts_dev->fwdata.rows * cts_dev->fwdata.cols);

        count += scnprintf(buf + count, PAGE_SIZE - count,
            SPLIT_LINE_STR
            "Diff data MIN: [%d][%d]=%d, MAX: [%d][%d]=%d, AVG=%d\n"
            SPLIT_LINE_STR
            "   |  ", min_r, min_c, min, max_r, max_c, max, average);
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            count += scnprintf(buf + count, PAGE_SIZE - count, COL_NUM_FORMAT_STR, c);
        }
        count += scnprintf(buf + count, PAGE_SIZE - count, "\n" SPLIT_LINE_STR);

        for (r = 0; r < cts_dev->fwdata.rows; r++) {
            count += scnprintf(buf + count, PAGE_SIZE - count, ROW_NUM_FORMAT_STR, r);
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                count += scnprintf(buf + count, PAGE_SIZE - count,
                    DATA_FORMAT_STR, diffdata[r * cts_dev->fwdata.cols + c]);
           }
           buf[count++] = '\n';
        }
    }

err_free_diffdata:
    cts_unlock_device(cts_dev);
    kfree(diffdata);

    return (data_valid ? count : ret);
}
static DEVICE_ATTR(diffdata, S_IRUGO, diffdata_show, NULL);

static ssize_t baseline_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#define BASELINE_BUFFER_SIZE(cts_dev) \
    (cts_dev->fwdata.rows * cts_dev->fwdata.cols * 2)

    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u16 *baseline = NULL;
    int ret, r, c, count = 0;
    u32 max, min, sum, average;
    int max_r, max_c, min_r, min_c;
    u16 addr = 0;
    u8  data_width = 0;

    TPD_INFO("<I> Show baseline\n");

    baseline = (u16 *)kmalloc(BASELINE_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (baseline == NULL) {
        return scnprintf(buf, PAGE_SIZE,
            "Alloc memory for baseline failed\n");
    }

    cts_lock_device(cts_dev);
    ret = cts_get_baseline(cts_dev, baseline, CTS_WORK_MODE_NORMAL_ACTIVE,
        CTS_GET_TOUCH_DATA_FLAG_ENABLE_GET_TOUCH_DATA_BEFORE |
        CTS_GET_TOUCH_DATA_FLAG_CLEAR_DATA_READY |
        CTS_GET_TOUCH_DATA_FLAG_REMOVE_TOUCH_DATA_BORDER |
        CTS_GET_TOUCH_DATA_FLAG_FLIP_TOUCH_DATA |
        CTS_GET_TOUCH_DATA_FLAG_DISABLE_GET_TOUCH_DATA_AFTER,
        addr, data_width);
    cts_unlock_device(cts_dev);

    if (ret) {
        kfree(baseline);
        return scnprintf(buf, PAGE_SIZE,
            "Get baseline failed %d(%s)\n", ret, cts_strerror(ret));
    }

    max = min = baseline[0];
    sum = 0;
    max_r = max_c = min_r = min_c = 0;
    for (r = 0; r < cts_dev->fwdata.rows; r++) {
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            u16 val = baseline[r * cts_dev->fwdata.cols + c];
            sum += val;
            if (val > max) {
                max = val;
                max_r = r;
                max_c = c;
            } else if (val < min) {
                min = val;
                min_r = r;
                min_c = c;
            }
        }
    }
    average = sum / (cts_dev->fwdata.rows * cts_dev->fwdata.cols);

    count += scnprintf(buf + count, PAGE_SIZE - count,
        SPLIT_LINE_STR
        "Baseline MIN: [%d][%d]=%u, MAX: [%d][%d]=%u, AVG=%u\n"
        SPLIT_LINE_STR
        "   |  ", min_r, min_c, min, max_r, max_c, max, average);
    for (c = 0; c < cts_dev->fwdata.cols; c++) {
        count += scnprintf(buf + count, PAGE_SIZE - count, COL_NUM_FORMAT_STR, c);
    }
    count += scnprintf(buf + count, PAGE_SIZE - count, "\n" SPLIT_LINE_STR);

    for (r = 0; r < cts_dev->fwdata.rows && count < PAGE_SIZE; r++) {
        count += scnprintf(buf + count, PAGE_SIZE - count, ROW_NUM_FORMAT_STR, r);
        for (c = 0; c < cts_dev->fwdata.cols && count < PAGE_SIZE; c++) {
            count += scnprintf(buf + count, PAGE_SIZE - count,
                DATA_FORMAT_STR, baseline[r * cts_dev->fwdata.cols + c]);
        }
        buf[count++] = '\n';
    }

    kfree(baseline);

    return count;

#undef BASELINE_BUFFER_SIZE
}
static DEVICE_ATTR(baseline, S_IRUGO, baseline_show, NULL);

static ssize_t manualdiffdata_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    s16 *rawdata = NULL;
    int ret, r, c, count = 0;
    int max, min, sum, average;
    int max_r, max_c, min_r, min_c;
    bool data_valid = true;
    int frame;
    struct file *file = NULL;
    int i;
    loff_t pos = 0;

    TPD_INFO("<I> Show manualdiff\n");

    if (argc != 1 && argc != 2) {
        return scnprintf(buf, PAGE_SIZE, "Invalid num args\n"
                "USAGE:\n"
                "  1. echo updatebase > manualdiff\n"
                "  2. cat manualdiff\n"
                "  or\n"
                "  1. echo updatebase > manualdiff\n"
                "  2. echo frame file > manualdiff\n"
                "  3. cat manualdiff\n");
    }

    if (argc == 2) {
        ret = kstrtou32(argv[0], 0, &frame);
        if (ret) {
            return scnprintf(buf, PAGE_SIZE, "Invalid frame num\n");
        }
        file = filp_open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (IS_ERR(file)) {
            return scnprintf(buf, PAGE_SIZE, "Can't open file:%s", argv[1]);
        }
    }
    else {
        frame = 1;
    }

    rawdata = (s16 *)kmalloc(DIFFDATA_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (rawdata == NULL) {
        TPD_INFO("<E> Allocate memory for rawdata failed\n");
        filp_close(file, NULL);
        return -ENOMEM;
    }

    cts_lock_device(cts_dev);
    ret = cts_enable_get_rawdata(cts_dev);
    if (ret) {
        TPD_INFO("<E> Enable read raw data failed %d\n", ret);
        goto err_free_diffdata;
    }

    ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
    if (ret) {
        TPD_INFO("<E> Send cmd QUIT_GESTURE_MONITOR failed %d\n", ret);
        goto err_free_diffdata;
    }
    msleep(50);

    TPD_INFO("<I> frame %d, file:%s\n", frame, argv[1]);
    for (i = 0; i < frame; i++) {
        ret = cts_get_rawdata(cts_dev, rawdata);
        if(ret) {
            TPD_INFO("<E> Get raw data failed %d\n", ret);
            data_valid = false;
        }
        else {
            data_valid = true;
        }
        msleep(50);

        if (data_valid) {
            max = -32768;
            min = 32767;
            sum = 0;
            max_r = max_c = min_r = min_c = 0;
            for (r = 0; r < cts_dev->fwdata.rows; r++) {
                for (c = 0; c < cts_dev->fwdata.cols; c++) {
                    s16 val;

                    rawdata[r * cts_dev->fwdata.cols + c] -= manualdiff_base[r * cts_dev->fwdata.cols + c];
                    val = rawdata[r * cts_dev->fwdata.cols + c];
                    sum += val;
                    if (val > max) {
                        max = val;
                        max_r = r;
                        max_c = c;
                    } else if (val < min) {
                        min = val;
                        min_r = r;
                        min_c = c;
                    }
                }
            }
            average = sum / (cts_dev->fwdata.rows * cts_dev->fwdata.cols);
            count += scnprintf(buf + count, PAGE_SIZE - count,
                SPLIT_LINE_STR
                "Manualdiff data MIN: [%d][%d]=%d, MAX: [%d][%d]=%d, AVG=%d\n"
                SPLIT_LINE_STR
                "   |  ", min_r, min_c, min, max_r, max_c, max, average);
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                count += scnprintf(buf + count,PAGE_SIZE - count,  COL_NUM_FORMAT_STR, c);
            }
            count += scnprintf(buf + count, PAGE_SIZE - count, "\n" SPLIT_LINE_STR);

            for (r = 0; r < cts_dev->fwdata.rows; r++) {
                count += scnprintf(buf + count, PAGE_SIZE - count, ROW_NUM_FORMAT_STR, r);
                for (c = 0; c < cts_dev->fwdata.cols; c++) {
                    count += scnprintf(buf + count, PAGE_SIZE - count,
                        DATA_FORMAT_STR, rawdata[r * cts_dev->fwdata.cols + c]);
               }
               buf[count++] = '\n';
            }
            //TPD_INFO("<I> manualdiffdata_show:%d, %d\n", i, frame);
            if (argc == 2) {
                pos = file->f_pos;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
                ret = kernel_write(file, buf, count, &pos);
#else
                ret = kernel_write(file, buf, count, pos);
#endif
                if (ret != count) {
                    TPD_INFO("<E> Write data to file '%s' failed %d\n", argv[1], ret);
                }
                file->f_pos += count;
                count = 0;
            }
        }
    }
    if (argc == 2) {
        filp_close(file, NULL);
    }

    ret = cts_disable_get_rawdata(cts_dev);
    if (ret) {
        TPD_INFO("<E> Disable read raw data failed %d\n", ret);
        // Fall through to show diffdata
    }

err_free_diffdata:
    cts_unlock_device(cts_dev);
    kfree(rawdata);

    return (data_valid ? count : ret);
}

static ssize_t manualdiffdata_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    int ret;

    parse_arg(buf, count);

    cts_lock_device(cts_dev);
    if (strncasecmp("updatebase", argv[0], 10) == 0) {
        ret = cts_enable_get_rawdata(cts_dev);
        if (ret) {
            TPD_INFO("<E> Enable read raw data failed %d\n", ret);
            goto err_manual_diff_store;
        }

        ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
        if (ret) {
            TPD_INFO("<E> Send cmd QUIT_GESTURE_MONITOR failed %d\n", ret);
            goto err_manual_diff_store;
        }
        msleep(50);

        if (manualdiff_base != NULL) {
            ret = cts_get_rawdata(cts_dev, manualdiff_base);
            if(ret) {
                TPD_INFO("<E> Get raw data failed %d\n", ret);
            }
        }
        //TPD_INFO("<I> update base successful\n");
        ret = cts_disable_get_rawdata(cts_dev);
        if (ret) {
            TPD_INFO("<E> Disable read raw data failed %d\n", ret);
        }
    }
err_manual_diff_store:
    cts_unlock_device(cts_dev);
    return count;
}

static DEVICE_ATTR(manualdiff, S_IRUSR|S_IWUSR, manualdiffdata_show, manualdiffdata_store);

static ssize_t jitter_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    u16 cnt;
    int ret;

    parse_arg(buf, count);

    if (argc != 1) {
        TPD_INFO("<E> Invalid num args %d\n", argc);
        return -EFAULT;
    }

    ret = kstrtou16(argv[0], 0, &cnt);
    if (ret == 0) {
        if (cnt > 2 || cnt < 10000) {
            jitter_test_frame = cnt;
        }
    }

    TPD_INFO("<I> jitter test frame: %d\n", jitter_test_frame);
    return count;
}

static ssize_t jitter_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    s16 *rawdata = NULL;
    s16 *rawdata_min = NULL;
    s16 *rawdata_max = NULL;
    int ret, r, c, count = 0;
    int max, min, sum, average;
    int max_r, max_c, min_r, min_c;
    bool data_valid = false;
    int i;

    TPD_INFO("<I> Show jitter\n");
    cts_lock_device(cts_dev);
    rawdata = (s16 *)kmalloc(DIFFDATA_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (rawdata == NULL) {
        TPD_INFO("<E> Allocate memory for rawdata failed\n");
        ret = -ENOMEM;
        goto err_jitter_show_exit;
    }
    rawdata_min = (s16 *)kmalloc(DIFFDATA_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (rawdata_min == NULL) {
        TPD_INFO("<E> Allocate memory for rawdata failed\n");
        ret = -ENOMEM;
        goto err_free_rawdata;
    }
    rawdata_max = (s16 *)kmalloc(DIFFDATA_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (rawdata_max == NULL) {
        TPD_INFO("<E> Allocate memory for rawdata failed\n");
        ret = -ENOMEM;
        goto err_free_rawdata_min;
    }

    for (i = 0; i < DIFFDATA_BUFFER_SIZE(cts_dev)/2; i++) {
        rawdata_min[i] = 32767;
        rawdata_max[i] = -32768;
    }

    ret = cts_enable_get_rawdata(cts_dev);
    if (ret) {
        TPD_INFO("<E> Enable read raw data failed %d\n", ret);
        goto err_free_rawdata_max;
    }

    ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
    if (ret) {
        TPD_INFO("<E> Send cmd QUIT_GESTURE_MONITOR failed %d\n", ret);
        goto err_free_rawdata_max;
    }
    msleep(50);
    data_valid = true;
    for (i = 0; i < jitter_test_frame; i++)
    {
        ret = cts_get_rawdata(cts_dev, rawdata);
        if(ret) {
            TPD_INFO("<E> Get raw data failed %d\n", ret);
            continue;
        }
        for (r = 0; r < cts_dev->fwdata.rows; r++) {
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                int index;
                index = r * cts_dev->fwdata.cols + c;
                if (rawdata_min[index] > rawdata[index])
                    rawdata_min[index] = rawdata[index];
                else if (rawdata_max[index] < rawdata[index])
                    rawdata_max[index] = rawdata[index];
            }
        }
        msleep(1);
    }
    ret = cts_disable_get_rawdata(cts_dev);
    if (ret) {
        TPD_INFO("<E> Disable read raw data failed %d\n", ret);
    }

    if (data_valid) {
        max = -32768;
        min = 32767;
        sum = 0;
        max_r = max_c = min_r = min_c = 0;
        for (r = 0; r < cts_dev->fwdata.rows; r++) {
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                s16 val;
                int index = r * cts_dev->fwdata.cols + c;

                val = rawdata_max[index] - rawdata_min[index];
                rawdata[index] = val;
                sum += val;
                if (val > max) {
                    max = val;
                    max_r = r;
                    max_c = c;
                } else if (val < min) {
                    min = val;
                    min_r = r;
                    min_c = c;
                }
            }
        }
        average = sum / (cts_dev->fwdata.rows * cts_dev->fwdata.cols);

        count += scnprintf(buf + count, PAGE_SIZE - count,
            SPLIT_LINE_STR
            "Jitter data MIN: [%d][%d]=%d, MAX: [%d][%d]=%d, AVG=%d, TOTAL FRAME=%d\n"
            SPLIT_LINE_STR
            "   |  ", min_r, min_c, min, max_r, max_c, max, average, jitter_test_frame);
        for (c = 0; c < cts_dev->fwdata.cols; c++) {
            count += scnprintf(buf + count, PAGE_SIZE - count, COL_NUM_FORMAT_STR, c);
        }
        count += scnprintf(buf + count, PAGE_SIZE - count, "\n" SPLIT_LINE_STR);

        for (r = 0; r < cts_dev->fwdata.rows; r++) {
            count += scnprintf(buf + count, PAGE_SIZE - count, ROW_NUM_FORMAT_STR, r);
            for (c = 0; c < cts_dev->fwdata.cols; c++) {
                count += scnprintf(buf + count, PAGE_SIZE - count,
                    DATA_FORMAT_STR, rawdata[r * cts_dev->fwdata.cols + c]);
           }
           buf[count++] = '\n';
        }
    }

err_free_rawdata_max:
    kfree(rawdata_max);
err_free_rawdata_min:
    kfree(rawdata_min);
err_free_rawdata:
    kfree(rawdata);
err_jitter_show_exit:
    cts_unlock_device(cts_dev);
    return (data_valid ? count : ret);
}

static DEVICE_ATTR(jitter, S_IRUSR|S_IWUSR, jitter_show, jitter_store);

static ssize_t compensate_cap_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u8 *cap = NULL;
    int ret;
    ssize_t count = 0;
    int r, c, min, max, max_r, max_c, min_r, min_c, sum, average;

    TPD_INFO("<I> Read '%s'\n", attr->attr.name);

    cap = kzalloc(cts_dev->hwdata->num_row * cts_dev->hwdata->num_col,
        GFP_KERNEL);
    if (cap == NULL) {
        return scnprintf(buf, PAGE_SIZE,
            "Allocate mem for compensate cap failed\n");
    }

    cts_lock_device(cts_dev);
    ret = cts_get_compensate_cap(cts_dev, cap);
    cts_unlock_device(cts_dev);
    if (ret) {
        kfree(cap);
        return scnprintf(buf, PAGE_SIZE,
            "Get compensate cap failed %d\n", ret);
    }

    max = min = cap[0];
    sum = 0;
    max_r = max_c = min_r = min_c = 0;
    for (r = 0; r < cts_dev->hwdata->num_row; r++) {
        for (c = 0; c < cts_dev->hwdata->num_col; c++) {
            u16 val = cap[r * cts_dev->hwdata->num_col + c];
            sum += val;
            if (val > max) {
                max = val;
                max_r = r;
                max_c = c;
            } else if (val < min) {
                min = val;
                min_r = r;
                min_c = c;
            }
        }
    }
    average = sum / (cts_dev->hwdata->num_row * cts_dev->hwdata->num_col);

    count += scnprintf(buf + count, PAGE_SIZE - count,
        "----------------------------------------------------------------------------\n"
        " Compensatete Cap MIN: [%d][%d]=%u, MAX: [%d][%d]=%u, AVG=%u\n"
        "---+------------------------------------------------------------------------\n"
        "   |", min_r, min_c, min, max_r, max_c, max, average);
    for (c = 0; c < cts_dev->hwdata->num_col; c++) {
        count += scnprintf(buf + count, PAGE_SIZE - count, " %3u", c);
    }
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "\n"
        "---+------------------------------------------------------------------------\n");

    for (r = 0; r < cts_dev->hwdata->num_row; r++) {
        count += scnprintf(buf + count, PAGE_SIZE - count, "%2u |", r);
        for (c = 0; c < cts_dev->hwdata->num_col; c++) {
            count += scnprintf(buf + count, PAGE_SIZE - count,
                " %3u", cap[r * cts_dev->hwdata->num_col + c]);
       }
       buf[count++] = '\n';
    }
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "---+------------------------------------------------------------------------\n");

    kfree(cap);

    return count;
}
static DEVICE_ATTR(compensate_cap, S_IRUGO, compensate_cap_show, NULL);

#ifdef CFG_CTS_HAS_RESET_PIN
#if 0
static ssize_t reset_pin_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    TPD_INFO("<I> Read RESET-PIN\n");

    return scnprintf(buf, PAGE_SIZE,
        "Reset pin: %d, status: %d\n",
        cts_data->pdata->rst_gpio,
        gpio_get_value(cts_data->pdata->rst_gpio));
}
#endif

static ssize_t reset_pin_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;

    TPD_INFO("<I> Write RESET-PIN\n");
    TPD_INFO("<I> Chip status maybe changed\n");

    cts_plat_set_reset(cts_dev->pdata, (buf[0] == '1') ? 1 : 0);
    return count;
}
static DEVICE_ATTR(reset_pin, S_IWUSR, NULL, reset_pin_store);
#endif

#if 0
static ssize_t irq_pin_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);

    TPD_INFO("<I> Read IRQ-PIN\n");

    return scnprintf(buf, PAGE_SIZE,
        "IRQ pin: %d, status: %d\n",
        cts_data->pdata->int_gpio,
        gpio_get_value(cts_data->pdata->int_gpio));
}
static DEVICE_ATTR(irq_pin, S_IRUGO, irq_pin_show, NULL);
#endif

static ssize_t irq_info_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct irq_desc *desc;

    TPD_INFO("<I> Read IRQ-INFO\n");

    desc = irq_to_desc(cts_data->pdata->irq);
    if (desc == NULL) {
        return scnprintf(buf, PAGE_SIZE, "IRQ: %d descriptor not found\n",
            cts_data->pdata->irq);
    }

    return scnprintf(buf, PAGE_SIZE,
        "IRQ num: %d, depth: %u, "
        "count: %u, unhandled: %u, last unhandled eslape: %lu\n",
        cts_data->pdata->irq, desc->depth,
        desc->irq_count, desc->irqs_unhandled,
        desc->last_unhandled);
}
static DEVICE_ATTR(irq_info, S_IRUGO, irq_info_show, NULL);

#ifdef CFG_CTS_FW_LOG_REDIRECT
static ssize_t fw_log_redirect_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;

    return scnprintf(buf, PAGE_SIZE, "Fw log redirect is %s\n",
        cts_is_fw_log_redirect(cts_dev)? "enable":"disable");
}

static ssize_t fw_log_redirect_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    u8 enable = 0;

    if (buf[0] == 'Y' || buf[0] == 'y' || buf[0] == '1') {
        enable = 1;
    }
    if (enable) {
        cts_enable_fw_log_redirect(cts_dev);
    } else {
        cts_disable_fw_log_redirect(cts_dev);
    }

    return count;
}

static DEVICE_ATTR(fw_log_redirect, S_IRUSR | S_IWUSR,
    fw_log_redirect_show, fw_log_redirect_store);
#endif


static struct attribute *cts_dev_misc_atts[] = {
    &dev_attr_ic_type.attr,
    &dev_attr_program_mode.attr,
    &dev_attr_rawdata.attr,
    &dev_attr_diffdata.attr,
    &dev_attr_baseline.attr,
    &dev_attr_manualdiff.attr,
    &dev_attr_jitter.attr,
#ifdef CFG_CTS_HAS_RESET_PIN
    &dev_attr_reset_pin.attr,
#endif
    //&dev_attr_irq_pin.attr,
    &dev_attr_irq_info.attr,
#ifdef CFG_CTS_FW_LOG_REDIRECT
    &dev_attr_fw_log_redirect.attr,
#endif
    &dev_attr_compensate_cap.attr,
    &dev_attr_read_reg.attr,
    &dev_attr_write_reg.attr,
    &dev_attr_read_hw_reg.attr,
    &dev_attr_write_hw_reg.attr,
    NULL
};

static const struct attribute_group cts_dev_misc_attr_group = {
    .name  = "misc",
    .attrs = cts_dev_misc_atts,
};

static const struct attribute_group *cts_dev_attr_groups[] = {
    &cts_dev_firmware_attr_group,
    &cts_dev_flash_attr_group,
    &cts_dev_test_attr_group,
    &cts_dev_misc_attr_group,
    NULL
};

int cts_sysfs_add_device(struct device *dev)
{
    struct chipone_ts_data *cts_data = cts_dev_get_drvdata(dev);
    struct cts_device *cts_dev = &cts_data->cts_dev;
    int ret = 0, i;

    TPD_INFO("<I> Add device attr groups\n");

    // Low version kernel NOT support sysfs_create_groups()
    for (i = 0; cts_dev_attr_groups[i]; i++) {
        ret = sysfs_create_group(&dev->kobj, cts_dev_attr_groups[i]);
        if (ret) {
            while (--i >= 0) {
                sysfs_remove_group(&dev->kobj, cts_dev_attr_groups[i]);
            }
            break;
        }
    }

    if (ret) {
        TPD_INFO("<E> Add device attr failed %d\n", ret);
        return ret;
    }

    manualdiff_base = (s16 *)kzalloc(DIFFDATA_BUFFER_SIZE(cts_dev), GFP_KERNEL);
    if (manualdiff_base == NULL) {
        TPD_INFO("<E> Malloc manualdiff_base failed\n");
        return -ENOMEM;
    }

    ret = sysfs_create_link(NULL, &dev->kobj, "chipone-tddi");
    if (ret) {
        TPD_INFO("<E> Create sysfs link error:%d\n", ret);
    }
    return 0;
}

void cts_sysfs_remove_device(struct device *dev)
{
    int i;

    TPD_INFO("<I> Remove device attr groups\n");

    if (manualdiff_base != NULL) {
        kfree(manualdiff_base);
        manualdiff_base = NULL;
    }

    sysfs_remove_link(NULL, "chipone-tddi");
    // Low version kernel NOT support sysfs_remove_groups()
    for (i = 0; cts_dev_attr_groups[i]; i++) {
        sysfs_remove_group(&dev->kobj, cts_dev_attr_groups[i]);
    }
}

#undef SPLIT_LINE_STR
#undef ROW_NUM_FORMAT_STR
#undef COL_NUM_FORMAT_STR
#undef DATA_FORMAT_STR

#endif /* CONFIG_CTS_SYSFS */
