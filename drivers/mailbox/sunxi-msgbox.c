// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2017-2019 wujiayi <wujiayi@allwinnertech.com>

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#define SUPPORT_TXDONE_IRQ	0

#define SUNXI_MSGBOX_PROCESSORS_MAX	2
#define SUNXI_MSGBOX_CHANNELS_MAX	4
#define SUNXI_MSGBOX_FIFO_MSG_NUM_MAX	8

#define SUNXI_MSGBOX_OFFSET(n)			(0x100 * (n))
#define SUNXI_MSGBOX_READ_IRQ_ENABLE(n)		(0x20 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_READ_IRQ_STATUS(n)		(0x24 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_WRITE_IRQ_ENABLE(n)	(0x30 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_WRITE_IRQ_STATUS(n)	(0x34 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_DEBUG_REGISTER(n)		(0x40 + SUNXI_MSGBOX_OFFSET(n))
#define SUNXI_MSGBOX_FIFO_STATUS(n, p)		(0x50 + SUNXI_MSGBOX_OFFSET(n) + 0x4 * (p))
#define SUNXI_MSGBOX_MSG_STATUS(n, p)		(0x60 + SUNXI_MSGBOX_OFFSET(n) + 0x4 * (p))
#define SUNXI_MSGBOX_MSG_FIFO(n, p)		(0x70 + SUNXI_MSGBOX_OFFSET(n) + 0x4 * (p))
#define SUNXI_MSGBOX_WRITE_IRQ_THRESHOLD(n, p)	(0x80 + SUNXI_MSGBOX_OFFSET(n) + 0x4 * (p))

/* SUNXI_MSGBOX_READ_IRQ_ENABLE */
#define RD_IRQ_EN_MASK			0x1
#define RD_IRQ_EN_SHIFT(p)		((p) * 2)

/* SUNXI_MSGBOX_READ_IRQ_STATUS */
#define RD_IRQ_PEND_MASK		0x1
#define RD_IRQ_PEND_SHIFT(p)		((p) * 2)

/* SUNXI_MSGBOX_WRITE_IRQ_ENABLE */
#define WR_IRQ_EN_MASK			0x1
#define WR_IRQ_EN_SHIFT(p)		((p) * 2 + 1)

/* SUNXI_MSGBOX_WRITE_IRQ_STATUS */
#define WR_IRQ_PEND_MASK		0x1
#define WR_IRQ_PEND_SHIFT(p)		((p) * 2 + 1)

/* SUNXI_MSGBOX_MSG_STATUS */
#define MSG_NUM_MASK			0xF
#define MSG_NUM_SHIFT			0

/* SUNXI_MSGBOX_WRITE_IRQ_THRESHOLD */
#define WR_IRQ_THR_MASK			0x3
#define WR_IRQ_THR_SHIFT		0

#define MBOX_NUM_CHANS (SUNXI_MSGBOX_CHANNELS_MAX * (SUNXI_MSGBOX_PROCESSORS_MAX - 1))

#define msgbox_dbg(msgbox, ...)	dev_dbg((msgbox)->controller.dev, __VA_ARGS__)

struct sunxi_msgbox {
	struct mbox_controller controller;
	struct clk *clk;
	void __iomem *regs[SUNXI_MSGBOX_PROCESSORS_MAX];
	int *irq;
	int irq_cnt;
	int local_id;
#ifdef CONFIG_PM_SLEEP
	uint8_t chan_status[MBOX_NUM_CHANS];
#endif
};

static bool sunxi_msgbox_peek_data(struct mbox_chan *chan);

static inline void reg_bits_set(void __iomem *reg, u32 mask, u32 shift)
{
	u32 val;

	val = readl(reg);
	val |= (mask << shift);
	writel(val, reg);
}

static inline void reg_bits_clear(void __iomem *reg, u32 mask, u32 shift)
{
	u32 val;

	val = readl(reg);
	val &= ~(mask << shift);
	writel(val, reg);
}

static inline u32 reg_bits_get(void __iomem *reg, u32 mask, u32 shift)
{
	return (readl(reg) & (mask << shift)) >> shift;
}

static inline void reg_val_update(void __iomem *reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;

	reg_val = readl(reg);
	reg_val &= ~(mask << shift);
	reg_val |= ((val & mask) << shift);
	writel(reg_val, reg);
}

/*
 * In sunxi msgbox, the coefficient N represents "the index of other processor
 * that communicates with local processor". For one specific local processor,
 * different coefficient N means differnt remote processor.
 *
 * For example, if we have 3 processors, numbered with ID 0~2, the relationship
 * of local_id, coefficient N (from the local processor side), remote_id is as
 * follows:
 *     --------------------------------------------
 *         local_id        N        remote_id
 *     --------------------------------------------
 *             0           0            1
 *             0           1            2
 *     --------------------------------------------
 *             1           0            0
 *             1           1            2
 *     --------------------------------------------
 *             2           0            0
 *             2           1            1
 *     --------------------------------------------
 */
static inline int sunxi_msgbox_coef_n(int local_id, int remote_id)
{
	return (local_id < remote_id) ? (remote_id - 1) : remote_id;
}

static inline int sunxi_msgbox_remote_id(int local_id, int coef_n)
{
	return (local_id >= coef_n) ? (coef_n + 1) : coef_n;
}

/*
 * For local processor, its mailbox channel index is defined from coefficient N
 * and coefficient P:
 *	mailbox channel index = N * SUNXI_MSGBOX_CHANNELS_MAX + P
 */
static inline void mbox_chan_id_to_coef_n_p(int mbox_chan_id,
					    int *coef_n, int *coef_p)
{
	*coef_n = mbox_chan_id / SUNXI_MSGBOX_CHANNELS_MAX;
	*coef_p = mbox_chan_id % SUNXI_MSGBOX_CHANNELS_MAX;
}

static inline void mbox_chan_to_coef_n_p(struct mbox_chan *chan,
					 int *coef_n, int *coef_p)
{
	mbox_chan_id_to_coef_n_p(chan - chan->mbox->chans, coef_n, coef_p);
}

static inline void *sunxi_msgbox_reg_base(struct sunxi_msgbox *msgbox, int index)
{
	void *base;

	WARN_ON(index >= SUNXI_MSGBOX_PROCESSORS_MAX);
	base = msgbox->regs[index];
	WARN_ON(!base);

	return base;
}

static inline struct sunxi_msgbox *to_sunxi_msgbox(struct mbox_chan *chan)
{
	return chan->con_priv;
}

static inline void sunxi_msgbox_set_write_irq_threshold(struct sunxi_msgbox *msgbox,
							void __iomem *base, int n, int p,
							int threshold)
{
	u32 thr_val;
	void __iomem *reg = base + SUNXI_MSGBOX_WRITE_IRQ_THRESHOLD(n, p);

	switch (threshold) {
	case 8:
		thr_val = 3;
		break;
	case 4:
		thr_val = 2;
		break;
	case 2:
		thr_val = 1;
		break;
	case 1:
		thr_val = 0;
		break;
	default:
		dev_warn(msgbox->controller.dev,
			 "Invalid write irq threshold (%d). Use 1 instead.",
			 threshold);
		thr_val = 0;
		break;
	}

	reg_val_update(reg, WR_IRQ_THR_MASK, WR_IRQ_THR_SHIFT, thr_val);
}

static void sunxi_msgbox_read_irq(struct sunxi_msgbox *msgbox, struct mbox_chan *chan,
				  void __iomem *base, int n, int p)
{
	u32 msg;

	while (sunxi_msgbox_peek_data(chan)) {
		msg = readl(base + SUNXI_MSGBOX_MSG_FIFO(n, p));

		msgbox_dbg(msgbox, "Channel %d from processor %d received: 0x%08x\n",
				p, sunxi_msgbox_remote_id(msgbox->local_id, n), msg);

		mbox_chan_received_data(chan, &msg);
	}

	/* The IRQ pending can be cleared only once the FIFO is empty. */
	reg_bits_set(base + SUNXI_MSGBOX_READ_IRQ_STATUS(n),
			RD_IRQ_PEND_MASK, RD_IRQ_PEND_SHIFT(p));
}

#if SUPPORT_TXDONE_IRQ
static void sunxi_msgbox_write_irq(struct sunxi_msgbox *msgbox, struct mbox_chan *chan,
				   void __iomem *base, int n, int p)
{
	/*
	 * In msgbox hardware, the write IRQ will be triggered if the empty
	 * level in FIFO reaches the write IRQ threshold. It means that there
	 * is empty space in FIFO for local processor to write. Here we use
	 * the write IRQ to indicate TX is done, to ensure that there is empty
	 * space in FIFO for next message to send.
	 */

	/* Disable write IRQ */
	reg_bits_clear(base + SUNXI_MSGBOX_WRITE_IRQ_ENABLE(n),
			WR_IRQ_EN_MASK, WR_IRQ_EN_SHIFT(p));

	mbox_chan_txdone(chan, 0);

	msgbox_dbg(msgbox, "Channel %d (remote N: %d) transmission done\n", p, n);

	/* Clear write IRQ pending */
	reg_bits_set(base + SUNXI_MSGBOX_WRITE_IRQ_STATUS(n),
			WR_IRQ_PEND_MASK, WR_IRQ_PEND_SHIFT(p));
}
#endif

static irqreturn_t sunxi_msgbox_irq(int irq, void *dev_id)
{
	struct sunxi_msgbox *msgbox = dev_id;
	struct mbox_chan *chan;
	int local_id, remote_id;
	int local_n, remote_n, p;
	void __iomem *read_reg_base;
	void __iomem *write_reg_base;
	u32 read_irq_en, read_irq_pending;
	u32 write_irq_en, write_irq_pending;
	int i;

	for (i = 0; i < MBOX_NUM_CHANS; ++i) {
		chan = &msgbox->controller.chans[i];

		mbox_chan_id_to_coef_n_p(i, &local_n, &p);
		local_id = msgbox->local_id;
		remote_id = sunxi_msgbox_remote_id(local_id, local_n);
		remote_n = sunxi_msgbox_coef_n(remote_id, local_id);

		read_reg_base = sunxi_msgbox_reg_base(msgbox, local_id);
		write_reg_base = sunxi_msgbox_reg_base(msgbox, remote_id);

		read_irq_en = reg_bits_get(
				read_reg_base + SUNXI_MSGBOX_READ_IRQ_ENABLE(local_n),
				RD_IRQ_EN_MASK, RD_IRQ_EN_SHIFT(p));
		read_irq_pending = reg_bits_get(
				read_reg_base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n),
				RD_IRQ_PEND_MASK, RD_IRQ_PEND_SHIFT(p));
		write_irq_en = reg_bits_get(
				write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n),
				WR_IRQ_EN_MASK, WR_IRQ_EN_SHIFT(p));
		write_irq_pending = reg_bits_get(
				write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_STATUS(remote_n),
				WR_IRQ_PEND_MASK, WR_IRQ_PEND_SHIFT(p));

		if (read_irq_en && read_irq_pending)
			sunxi_msgbox_read_irq(msgbox, chan, read_reg_base, local_n, p);

#if SUPPORT_TXDONE_IRQ
		if (write_irq_en && write_irq_pending)
			sunxi_msgbox_write_irq(msgbox, chan, write_reg_base, remote_n, p);
#endif
	}

	return IRQ_HANDLED;
}

static int sunxi_msgbox_send_data(struct mbox_chan *chan, void *data)
{
	struct sunxi_msgbox *msgbox = to_sunxi_msgbox(chan);
	int local_id = msgbox->local_id;
	int local_n, remote_n, p;
	int remote_id;
	void __iomem *base;
	u32 msg_num;

	/*
	 * TODO:
	 *   Here we consider the data is always a pointer to u32.
	 *   Should we define a data structure, e.g. 'struct sunxi_mailbox_message',
	 *   to hide the actual data type for mailbox client users?
	 */
	u32 msg = *(u32 *)data;

	mbox_chan_to_coef_n_p(chan, &local_n, &p);
	remote_id = sunxi_msgbox_remote_id(local_id, local_n);
	remote_n = sunxi_msgbox_coef_n(remote_id, local_id);
	base = sunxi_msgbox_reg_base(msgbox, remote_id);

	/*
	 * Check whether FIFO is full.
	 *
	 * Ordinarily the 'tx_done' of previous message ensures the FIFO has
	 * empty space for this message to send. But in case the FIFO is already
	 * full before sending the first message, we check the number of messages
	 * in FIFO anyway.
	 */
	msg_num = reg_bits_get(base + SUNXI_MSGBOX_MSG_STATUS(remote_n, p),
			MSG_NUM_MASK, MSG_NUM_SHIFT);
	if (msg_num >= SUNXI_MSGBOX_FIFO_MSG_NUM_MAX) {
		msgbox_dbg(msgbox, "Channel %d to processor %d: FIFO is full\n",
				p, remote_id);
		return -EBUSY;
	}

	/* Write message to FIFO */
	writel(msg, base + SUNXI_MSGBOX_MSG_FIFO(remote_n, p));

#if SUPPORT_TXDONE_IRQ
	/*
	 * Enable write IRQ after writing message to FIFO, because we use the
	 * write IRQ to indicate whether FIFO has empty space for "next message"
	 * rather than "this message" to send.
	 */
	reg_bits_set(base + SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n),
			WR_IRQ_EN_MASK, WR_IRQ_EN_SHIFT(p));
#endif

	msgbox_dbg(msgbox, "Channel %d to processor %d sent: 0x%08x\n", p, remote_id, msg);

	return 0;
}

static int sunxi_msgbox_startup(struct mbox_chan *chan)
{
	struct sunxi_msgbox *msgbox = to_sunxi_msgbox(chan);
	int local_id, remote_id;
	int local_n, remote_n, p;
	void __iomem *read_reg_base;
	void __iomem *write_reg_base;

	mbox_chan_to_coef_n_p(chan, &local_n, &p);
	local_id = msgbox->local_id;
	remote_id = sunxi_msgbox_remote_id(local_id, local_n);
	remote_n = sunxi_msgbox_coef_n(remote_id, local_id);
	read_reg_base = sunxi_msgbox_reg_base(msgbox, local_id);
	write_reg_base = sunxi_msgbox_reg_base(msgbox, remote_id);

	/* Flush read FIFO */
	while (sunxi_msgbox_peek_data(chan))
		readl(read_reg_base + SUNXI_MSGBOX_MSG_FIFO(local_n, p));

	/* Clear read IRQ pending */
	reg_bits_set(read_reg_base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n),
			RD_IRQ_PEND_MASK, RD_IRQ_PEND_SHIFT(p));

	/* Enable read IRQ */
	reg_bits_set(read_reg_base + SUNXI_MSGBOX_READ_IRQ_ENABLE(local_n),
			RD_IRQ_EN_MASK, RD_IRQ_EN_SHIFT(p));

	/* Clear write IRQ pending */
	reg_bits_set(write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_STATUS(remote_n),
			WR_IRQ_PEND_MASK, WR_IRQ_PEND_SHIFT(p));

	/*
	 * Configure the FIFO empty level to trigger the write IRQ to 1.
	 * It means that if the write IRQ is enabled, once the FIFO is not full,
	 * the write IRQ will be triggered.
	 */
	sunxi_msgbox_set_write_irq_threshold(msgbox, write_reg_base, remote_n, p, 1);

#ifdef CONFIG_PM_SLEEP
	msgbox->chan_status[chan - chan->mbox->chans] = 1;
	msgbox_dbg(msgbox, "Channel %d enabled\n", chan - chan->mbox->chans);
#endif

	msgbox_dbg(msgbox, "Channel %d to remote %d startup complete\n",
			p, remote_id);

	return 0;
}

static void sunxi_msgbox_shutdown(struct mbox_chan *chan)
{
	struct sunxi_msgbox *msgbox = to_sunxi_msgbox(chan);
	int local_id, remote_id;
	int local_n, remote_n, p;
	void __iomem *read_reg_base;
	void __iomem *write_reg_base;

	mbox_chan_to_coef_n_p(chan, &local_n, &p);
	local_id = msgbox->local_id;
	remote_id = sunxi_msgbox_remote_id(local_id, local_n);
	remote_n = sunxi_msgbox_coef_n(remote_id, local_id);
	read_reg_base = sunxi_msgbox_reg_base(msgbox, local_id);
	write_reg_base = sunxi_msgbox_reg_base(msgbox, remote_id);

	/* Disable the read IRQ */
	reg_bits_clear(read_reg_base + SUNXI_MSGBOX_READ_IRQ_ENABLE(local_n),
			RD_IRQ_EN_MASK, RD_IRQ_EN_SHIFT(p));

	/* Attempt to flush the receive FIFO until the IRQ is cleared. */
	do {
		while (sunxi_msgbox_peek_data(chan))
			readl(read_reg_base + SUNXI_MSGBOX_MSG_FIFO(local_n, p));
		reg_bits_set(read_reg_base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n),
				RD_IRQ_PEND_MASK, RD_IRQ_PEND_SHIFT(p));
	} while (reg_bits_get(read_reg_base + SUNXI_MSGBOX_READ_IRQ_STATUS(local_n),
			RD_IRQ_PEND_MASK, RD_IRQ_PEND_SHIFT(p)));

	/* Disable the write IRQ */
	reg_bits_clear(write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n),
			WR_IRQ_EN_MASK, WR_IRQ_EN_SHIFT(p));

	/* Clear write IRQ pending */
	reg_bits_set(write_reg_base + SUNXI_MSGBOX_WRITE_IRQ_STATUS(remote_n),
			WR_IRQ_PEND_MASK, WR_IRQ_PEND_SHIFT(p));

#ifdef CONFIG_PM_SLEEP
	msgbox->chan_status[chan - chan->mbox->chans] = 0;
	msgbox_dbg(msgbox, "Channel %d disabled\n", chan - chan->mbox->chans);
#endif

	msgbox_dbg(msgbox, "Channel %d to remote %d shutdown complete\n",
			p, remote_id);
}

#if (!SUPPORT_TXDONE_IRQ)
static bool sunxi_msgbox_last_tx_done(struct mbox_chan *chan)
{
	/*
	 * Here we consider a transmission is done if the FIFO is not full.
	 * This ensures that the next message can be written to FIFO, and local
	 * processor need not to wait until remote processor has read this
	 * message from FIFO.
	 */
	struct sunxi_msgbox *msgbox = to_sunxi_msgbox(chan);
	int local_id = msgbox->local_id;
	int local_n, remote_n, p;
	int remote_id;
	void __iomem *status_reg;
	u32 msg_num;

	mbox_chan_to_coef_n_p(chan, &local_n, &p);
	remote_id = sunxi_msgbox_remote_id(local_id, local_n);
	remote_n = sunxi_msgbox_coef_n(remote_id, local_id);

	status_reg = sunxi_msgbox_reg_base(msgbox, remote_id) +
		SUNXI_MSGBOX_MSG_STATUS(remote_n, p);

	msg_num = reg_bits_get(status_reg, MSG_NUM_MASK, MSG_NUM_SHIFT);

	return msg_num < SUNXI_MSGBOX_FIFO_MSG_NUM_MAX ? true : false;
}
#endif

static bool sunxi_msgbox_peek_data(struct mbox_chan *chan)
{
	struct sunxi_msgbox *msgbox = to_sunxi_msgbox(chan);
	int n, p;
	void __iomem *status_reg;
	u32 msg_num;

	mbox_chan_to_coef_n_p(chan, &n, &p);
	status_reg = sunxi_msgbox_reg_base(msgbox, msgbox->local_id) +
		SUNXI_MSGBOX_MSG_STATUS(n, p);

	msg_num = reg_bits_get(status_reg, MSG_NUM_MASK, MSG_NUM_SHIFT);

	return msg_num > 0 ? true : false;
}

static const struct mbox_chan_ops sunxi_msgbox_chan_ops = {
	.send_data    = sunxi_msgbox_send_data,
	.startup      = sunxi_msgbox_startup,
	.shutdown     = sunxi_msgbox_shutdown,
#if SUPPORT_TXDONE_IRQ
	.last_tx_done = NULL,
#else
	.last_tx_done = sunxi_msgbox_last_tx_done,
#endif
	.peek_data    = sunxi_msgbox_peek_data,
};

#ifdef CONFIG_PM_SLEEP
static int sunxi_msgbox_suspend(struct device *dev)
{
	struct sunxi_msgbox *msgbox;

	msgbox = dev_get_drvdata(dev);

	clk_disable_unprepare(msgbox->clk);

	msgbox_dbg(msgbox, "Suspend\n");

	return 0;
}

static int sunxi_msgbox_resume(struct device *dev)
{
	struct mbox_chan *chan;
	struct sunxi_msgbox *msgbox;
	int i;
	int ret;

	msgbox = dev_get_drvdata(dev);

	ret = clk_prepare_enable(msgbox->clk);
	if (ret) {
		dev_err(dev, "Failed to enable clock: %d\n", ret);
		return ret;
	}

	msgbox_dbg(msgbox, "Resume\n");

	for (i = 0; i < MBOX_NUM_CHANS; i++) {
		chan = &msgbox->controller.chans[i];

		if (msgbox->chan_status[i])
			sunxi_msgbox_startup(chan);
		else
			sunxi_msgbox_shutdown(chan);
	}

	return 0;
}

static struct dev_pm_ops sunxi_msgbox_pm_ops = {
	.suspend = sunxi_msgbox_suspend,
	.resume = sunxi_msgbox_resume,
};
#endif

static int sunxi_msgbox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mbox_chan *chans;
	struct resource *res;
	struct sunxi_msgbox *msgbox;
	int i, j, ret;
	u32 tmp;

	msgbox = devm_kzalloc(dev, sizeof(*msgbox), GFP_KERNEL);
	if (!msgbox)
		return -ENOMEM;

	chans = devm_kcalloc(dev, MBOX_NUM_CHANS, sizeof(*chans), GFP_KERNEL);
	if (!chans)
		return -ENOMEM;

	for (i = 0; i < MBOX_NUM_CHANS; ++i)
		chans[i].con_priv = msgbox;

	msgbox->clk = of_clk_get(dev->of_node, 0);
	if (IS_ERR(msgbox->clk)) {
		ret = PTR_ERR(msgbox->clk);
		dev_err(dev, "Failed to get clock: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(msgbox->clk);
	if (ret) {
		dev_err(dev, "Failed to enable clock: %d\n", ret);
		return ret;
	}

#ifdef CONFIG_PM_SLEEP
	memset(msgbox->chan_status, 0, MBOX_NUM_CHANS);
#endif

	for (i = 0;  i < SUNXI_MSGBOX_PROCESSORS_MAX; ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(dev, "Failed to get resource %d\n", i);
			ret = -ENODEV;
			goto err_disable_unprepare;
		}

		msgbox->regs[i] = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(msgbox->regs[i])) {
			ret = PTR_ERR(msgbox->regs[i]);
			dev_err(dev, "Failed to map MMIO resource %d: %d\n", i, ret);
			goto err_disable_unprepare;
		}
	}

	ret = of_property_read_u32(dev->of_node, "local_id", &tmp);
	if (tmp < 0) {
		dev_err(dev, "Failed to get local_id\n");
		ret = -EINVAL;
		goto err_disable_unprepare;
	}
	msgbox->local_id = tmp;

	msgbox->irq_cnt = of_irq_count(dev->of_node);
	msgbox->irq = devm_kcalloc(dev, msgbox->irq_cnt, sizeof(int), GFP_KERNEL);
	if (!msgbox->irq) {
		ret = -ENOMEM;
		goto err_disable_unprepare;
	}

	for (i = 0; i < msgbox->irq_cnt; ++i) {
		ret = of_irq_get(dev->of_node, i);
		if (ret < 0) {
			dev_err(dev, "of_irq_get [%d] failed: %d\n", i, ret);
			goto err_disable_unprepare;
		} else if (ret == 0) {
			dev_err(dev, "of_irq_get [%d] map error\n", i);
			ret = -EINVAL;
			goto err_disable_unprepare;
		}
		msgbox->irq[i] = ret;
	}

	/* Disable all IRQs for this end of the msgbox. */
	for (i = 0; i < SUNXI_MSGBOX_PROCESSORS_MAX - 1; ++i) {
		int local_n = i;
		int local_id = msgbox->local_id;
		int remote_id = sunxi_msgbox_remote_id(local_id, local_n);
		int remote_n = sunxi_msgbox_coef_n(remote_id, local_id);

		void __iomem *read_reg_base = sunxi_msgbox_reg_base(msgbox, local_id);
		void __iomem *write_reg_base = sunxi_msgbox_reg_base(msgbox, remote_id);

		void __iomem *read_irq_en_reg = read_reg_base +
			SUNXI_MSGBOX_READ_IRQ_ENABLE(local_n);
		void __iomem *write_irq_en_reg = write_reg_base +
			SUNXI_MSGBOX_WRITE_IRQ_ENABLE(remote_n);

		u32 read_irq_en_value = readl(read_irq_en_reg);
		u32 write_irq_en_value = readl(write_irq_en_reg);

		for (j = 0; j < SUNXI_MSGBOX_CHANNELS_MAX; ++j) {
			read_irq_en_value &= ~(RD_IRQ_EN_MASK << RD_IRQ_EN_SHIFT(j));
			write_irq_en_value &= ~(WR_IRQ_EN_MASK << WR_IRQ_EN_SHIFT(j));
		}

		writel(read_irq_en_value, read_irq_en_reg);
		writel(write_irq_en_value, write_irq_en_reg);
	}

	for (i = 0; i < msgbox->irq_cnt; ++i) {
		ret = devm_request_irq(dev, msgbox->irq[i], sunxi_msgbox_irq,
				       0, dev_name(dev), msgbox);
		if (ret) {
			dev_err(dev, "Failed to register IRQ handler %d: %d\n", i, ret);
			goto err_disable_unprepare;
		}
	}

	msgbox->controller.dev           = dev;
	msgbox->controller.ops           = &sunxi_msgbox_chan_ops;
	msgbox->controller.chans         = chans;
	msgbox->controller.num_chans     = MBOX_NUM_CHANS;
#if SUPPORT_TXDONE_IRQ
	msgbox->controller.txdone_irq    = true;
#else
	msgbox->controller.txdone_irq    = false;
	msgbox->controller.txdone_poll   = true;
	msgbox->controller.txpoll_period = 5;
#endif

	platform_set_drvdata(pdev, msgbox);

	ret = mbox_controller_register(&msgbox->controller);
	if (ret) {
		dev_err(dev, "Failed to register controller: %d\n", ret);
		goto err_disable_unprepare;
	}

	return 0;

err_disable_unprepare:
	clk_disable_unprepare(msgbox->clk);

	return ret;
}

static int sunxi_msgbox_remove(struct platform_device *pdev)
{
	struct sunxi_msgbox *msgbox = platform_get_drvdata(pdev);

	mbox_controller_unregister(&msgbox->controller);
	/* See the comment in sunxi_msgbox_probe about the reset line. */
	clk_disable_unprepare(msgbox->clk);

	return 0;
}

static const struct of_device_id sunxi_msgbox_of_match[] = {
	{ .compatible = "allwinner,sunxi-msgbox", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_msgbox_of_match);

static struct platform_driver sunxi_msgbox_driver = {
	.driver = {
		.name = "sunxi-msgbox",
		.of_match_table = sunxi_msgbox_of_match,
#ifdef CONFIG_PM_SLEEP
		.pm = &sunxi_msgbox_pm_ops,
#endif
	},
	.probe  = sunxi_msgbox_probe,
	.remove = sunxi_msgbox_remove,
};

static int __init sunxi_mailbox_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_msgbox_driver);

	return ret;
}

static void __exit sunxi_mailbox_exit(void)
{
	platform_driver_unregister(&sunxi_msgbox_driver);
}

postcore_initcall(sunxi_mailbox_init);
module_exit(sunxi_mailbox_exit);

MODULE_DESCRIPTION("Sunxi Msgbox Driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
