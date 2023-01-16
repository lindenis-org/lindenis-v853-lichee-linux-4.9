/*
 * Espressif Systems Wireless LAN device driver
 *
 * Copyright (C) 2015-2021 Espressif Systems (Shanghai) PTE LTD
 *
 * This software file (the "File") is distributed by Espressif Systems (Shanghai)
 * PTE LTD under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available by writing to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
 * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "esp.h"
#include "esp_rb.h"
#include "esp_api.h"

#define ESP_SERIAL_MAJOR	221
#define ESP_SERIAL_MINOR_MAX	1
#define ESP_RX_RB_SIZE	4096

//#define ESP_SERIAL_TEST

#define IOCTL_ESP_FLUSH_RB _IOW('r', 1, unsigned long)

static struct esp_serial_devs {
	struct cdev cdev;
	int dev_index;
	esp_rb_t rb;
	void *priv;
	struct mutex lock;
} devs[ESP_SERIAL_MINOR_MAX];

static int esp_serial_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
	struct esp_serial_devs *dev = NULL;
	size_t ret_size = 0;
	dev = (struct esp_serial_devs *) file->private_data;
	ret_size = esp_rb_read_by_user(&dev->rb, user_buffer, size, !(file->f_flags & O_NONBLOCK));
	if (ret_size == 0) {
		printk(KERN_INFO "ESP serial read exp len %d ret_size ZERO \n", size);
		return -EAGAIN;
	}
	printk(KERN_INFO "ESP serial read exp len %d ret_size %d\n", size, ret_size);
	return ret_size;
}

static int esp_serial_write(struct file *file, const char __user *user_buffer, size_t size, loff_t * offset)
{
	struct esp_payload_header *hdr = NULL;
	u8 *tx_buf = NULL;
	struct esp_serial_devs *dev = NULL;
	struct sk_buff *tx_skb = NULL;
	int ret = 0;
	size_t total_len = 0;

	dev = (struct esp_serial_devs *) file->private_data;
	total_len = size + sizeof(struct esp_payload_header);

	tx_skb = esp_alloc_skb(total_len);
	if (!tx_skb) {
		printk (KERN_ERR "%s: SKB alloc failed\n", __func__);
		return -ENOMEM;
	}

	tx_buf = skb_put(tx_skb, total_len);

	hdr = (struct esp_payload_header *) tx_buf;

	memset (hdr, 0, sizeof(struct esp_payload_header));

	hdr->if_type = ESP_SERIAL_IF;
	hdr->if_num = dev->dev_index;
	hdr->len = cpu_to_le16(size);
	hdr->offset = cpu_to_le16(sizeof(struct esp_payload_header));

	ret = copy_from_user(tx_buf + hdr->offset, user_buffer, size);
	if (ret) {
		dev_kfree_skb(tx_skb);
		printk(KERN_ERR "%s, Error copying buffer to send serial data\n", __func__);
		return -EFAULT;
	}

	ret = esp_send_packet(dev->priv, tx_skb);
	if (ret) {
		printk (KERN_ERR "%s: Failed to transmit data\n", __func__);
	}

	return size;
}

static void esp_serial_cleanup(int dev_index)
{
	esp_rb_cleanup(&devs[0].rb);

	return;
}

static long esp_serial_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long val = 0;
	switch (cmd) {

	case IOCTL_ESP_FLUSH_RB:

		if (copy_from_user(&val, (unsigned long *) arg, sizeof(val))) {
			printk(KERN_ERR "IOCTL err while flush RB\n");
			}
		esp_serial_cleanup(val);
		break;

	default:
		printk(KERN_INFO "%s IOCTL %d\n", __func__, cmd);
		break;
	}
	return 0;
}

static int esp_serial_open(struct inode *inode, struct file *file)
{
	struct esp_serial_devs *devs = NULL;

	devs = container_of(inode->i_cdev, struct esp_serial_devs, cdev);
	file->private_data = devs;

	return 0;
}

static unsigned int esp_serial_poll(struct file *file, poll_table *wait)
{
    struct esp_serial_devs *dev = (struct esp_serial_devs *)file->private_data;
    unsigned int mask = 0;

    mutex_lock(&dev->lock);
    poll_wait(file, &dev->rb.wq,  wait);

    if (dev->rb.rp != dev->rb.wp) {
        mask |= (POLLIN | POLLRDNORM) ;   /* readable */
    }
    if (get_free_space(&dev->rb)) {
        mask |= (POLLOUT | POLLWRNORM) ;  /* writable */
    }

    mutex_unlock(&dev->lock);

    printk(KERN_INFO "ESP serial poll returned mask %u\n", mask);
    return mask;
}

const struct file_operations esp_serial_fops = {
	.owner = THIS_MODULE,
	.open = esp_serial_open,
	.read = esp_serial_read,
	.write = esp_serial_write,
	.unlocked_ioctl = esp_serial_ioctl,
	.poll = esp_serial_poll
};

int esp_serial_data_received(int dev_index, const char *data, size_t len)
{
	int ret = 0;
	size_t ret_len = 0;

	while (ret_len != len) {
		ret = esp_rb_write_by_kernel(&devs[dev_index].rb,
				data+ret_len, (len-ret_len));
		if (ret <= 0) {
			break;
		}
		ret_len += ret;
	}
	if (ret_len != len) {
		printk(KERN_ERR "ESP serial: RB: len %d ret_len %d ret %d\n", len, ret_len, ret);
	}
	printk(KERN_INFO "%s, write bytes asked %d, actual wrote %d\n", __func__, len, ret_len);
	return ret_len;
}

#ifdef ESP_SERIAL_TEST
static int thread_fn(void *unused)
{
	int i = 100;

	while(i--) {
		esp_rb_write_by_kernel(&devs[0].rb, "alphabetagamma", 14);
		ssleep(1);
	}
	printk(KERN_INFO "%s, Thread stopping\n", __func__);
	do_exit(0);
	return 0;
}
#endif

int esp_serial_init(void *priv)
{
	int err = 0, i = 0;

	err = register_chrdev_region(MKDEV(ESP_SERIAL_MAJOR, 0), ESP_SERIAL_MINOR_MAX, "esp_serial_driver");
	if (err) {
		printk(KERN_ERR "%s, Error registering chrdev region %d\n", __func__, err);
		return -1;
	}

	for (i = 0; i < ESP_SERIAL_MINOR_MAX; i++) {
		cdev_init(&devs[i].cdev, &esp_serial_fops);
		devs[i].dev_index = i;
		cdev_add(&devs[i].cdev, MKDEV(ESP_SERIAL_MAJOR, i), 1);
		esp_rb_init(&devs[i].rb, ESP_RX_RB_SIZE);
		devs[i].priv = priv;
		mutex_init(&devs[i].lock);
	}

#ifdef ESP_SERIAL_TEST
	kthread_run(thread_fn, NULL, "esptest-thread");
#endif
	return 0;
}

void esp_serial_del(void)
{
	int i = 0;
	for (i = 0; i < ESP_SERIAL_MINOR_MAX; i++) {
		cdev_del(&devs[i].cdev);
		esp_rb_del(&devs[i].rb);
		mutex_destroy(&devs[i].lock);
	}
	unregister_chrdev_region(MKDEV(ESP_SERIAL_MAJOR, 0), ESP_SERIAL_MINOR_MAX);
	return;
}
