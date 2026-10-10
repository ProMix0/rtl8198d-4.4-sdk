#define DEBUG

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <soc/cortina/registers.h>
#include <sound/ca77xx_snd_audio.h>

#include "ca77xx_snd_audio_reg.h"

#define DRV_NAME	"ca77xx-audio"

#define BUG48545	1	/* turn off own and update for test */

#define MAX_SLOT_NUM	128

#define CA77XX_DMA_SSP_DEPTH_MIN	3
#define CA77XX_DMA_SSP_DEPTH_MAX	13

#define DMA_DESC_ALIGN	16
#define DMA_BUF_ALIGN	8

#define OWN_BY_DMA	0
#define OWN_BY_CPU	1

#define CLK_UPBOUND	0xFFFFFFFF

#define powerof2(x)     ((x != 0) && ((x & (x - 1)) == 0))
#define div_up(a, b)	((a - 1) / b + 1)

struct ca77xx_audio_ssp_statis {
	u16 tfsFull;
	u16 tfsEmpty;
	u16 tfwWtMk;
	u16 tfsUndRun;
	u16 rfsFull;
	u16 rfsEmpty;
	u16 rfrWtMk;
	u16 rfsOvrRun;
	u16 slic_int;
	u16 pcm_slv_disc;
};

struct ca77xx_audio_ssp_ctrl {
	struct ca77xx_audio_ssp_statis statis;

	u8 type;	/* PCM, I2S */
	u8 flag;
	u8 spi_cs;
	u8 ext_clk;
	u8 clk_enhance;
	u8 clk_src_pol;
	u8 slave_fs;
};

struct ca77xx_audio_dma_desc {
	u32 buf_paddr;
	u32 buf_paddr_39_32:8;
	u32 buf_size:16;
	u32 sgm:1;
	u32 reserved:6;
	u32 own:1;
};

struct ca77xx_audio_dma_cnt {
	u16 rxq_full;
	u16 rxq_overrun;
	u16 rxq_cntmsb;
	u16 rxq_drop_overrun;
	u16 rxq_drop_cntmsb;
	u16 txq_empty;
	u16 txq_overrun;
	u16 txq_cntmsb;
};

struct ca77xx_audio_dma_ctrl {
	dma_addr_t tx_desc_paddr;
	dma_addr_t rx_desc_paddr;
	struct ca77xx_audio_dma_desc *tx_dma_desc;
	struct ca77xx_audio_dma_desc *rx_dma_desc;

	dma_addr_t tx_buf_paddr;
	dma_addr_t rx_buf_paddr;

	void *tx_buf;
	void *rx_buf;

	u16 tx_buf_num;
	u16 rx_buf_num;
	u16 tx_buf_size;
	u16 rx_buf_size;

	int tx_write_ptr;
	int rx_read_ptr;

	void (*dma_tx_notifier)(void *data);
	void *tx_data;
	void (*dma_rx_notifier)(void *data);
	void *rx_data;

	u32 dma_irq_tx_status;
	u32 dma_irq_rx_status;

	struct ca77xx_audio_dma_cnt cnt;

	u8 tx_swap;
	u8 rx_swap;
};

struct ca77xx_audio {
	struct device	*dev;

	void __iomem	*ssp_base;
	int		irq_ssp;
	struct clk	*clk;
	struct ca77xx_audio_ssp_ctrl ssp_ctrl;

	void __iomem	*dma_glb_base;
	void __iomem	*dma_ssp_base;
	int		irq_dma_tx;
	int		irq_dma_rx;
	struct ca77xx_audio_dma_ctrl dma_ctrl;

	spinlock_t	lock; /* lock access to registers */
};

#ifdef CONFIG_RTL8686_IPC_DEV
#ifdef G3_TEST
extern void aipc_audio_dma_txrx_notify(void);
#endif
#endif

static void ca77xx_audio_dma_cnt_init(struct ca77xx_audio *audio, int dir);
static void ca77xx_audio_dma_cnt_disp(struct ca77xx_audio *audio, int dir);
static int ca77xx_audio_dma_buf_check(u32 num, u32 size);
static int ca77xx_audio_dma_tx_update(struct ca77xx_audio *audio, u16 new_wt_idx);
static int ca77xx_audio_dma_rx_update(struct ca77xx_audio *audio, u16 new_rd_idx);
static int ca77xx_audio_dma_tx_enable(struct ca77xx_audio *audio);
static int ca77xx_audio_dma_rx_enable(struct ca77xx_audio *audio);
static int ca77xx_audio_dma_tx_disable(struct ca77xx_audio *audio);
static int ca77xx_audio_dma_rx_disable(struct ca77xx_audio *audio);
static int ca77xx_audio_dma_tx_idx(struct ca77xx_audio *audio, u16 *wt_idx_p, u16 *rd_idx_p);
static int ca77xx_audio_dma_rx_idx(struct ca77xx_audio *audio, u16 *wt_idx_p, u16 *rd_idx_p);
unsigned int ca77xx_snd_audio_dma_curr_offset(void *dev_instance, int dir);

#define AUDIO_REG_ACCESS_RD	0x00000001
#define AUDIO_REG_ACCESS_WR	0x00000002
#define AUDIO_FUNC_ENTRY	0x00000004
#define AUDIO_CONFIG		0x00000008

static u32 dbg_flag;
static char pattern;

static inline u32 ca77xx_rd_ssp_reg(struct ca77xx_audio *audio,
				    unsigned int reg)
{
	if (dbg_flag & AUDIO_REG_ACCESS_RD) {
		u32 val = readl(audio->ssp_base + reg);

		dev_dbg(audio->dev, "R, ssp, reg=0x%02x, val=0x%08x\n", reg,
			val);

		return val;
	}

	return readl(audio->ssp_base + reg);
}

static inline void ca77xx_wr_ssp_reg(struct ca77xx_audio *audio,
				     unsigned int reg, u32 val)
{
	if (dbg_flag & AUDIO_REG_ACCESS_WR)
		dev_dbg(audio->dev, "W, ssp, reg=0x%02x, val=0x%08x\n", reg,
			val);

	writel(val, audio->ssp_base + reg);
}

static inline u32 ca77xx_rd_dma_glb_reg(struct ca77xx_audio *audio,
					unsigned int reg)
{
	if (dbg_flag & AUDIO_REG_ACCESS_RD) {
		u32 val = readl(audio->dma_glb_base + reg);

		dev_dbg(audio->dev, "R, dma-glb, reg=0x%02x, val=0x%08x\n", reg,
			val);

		return val;
	}

	return readl(audio->dma_glb_base + reg);
}

static inline void ca77xx_wr_dma_glb_reg(struct ca77xx_audio *audio,
					 unsigned int reg, u32 val)
{
	if (dbg_flag & AUDIO_REG_ACCESS_WR)
		dev_dbg(audio->dev, "W, dma-glb, reg=0x%02x, val=0x%08x\n", reg,
			val);

	writel(val, audio->dma_glb_base + reg);
}

static inline u32 ca77xx_rd_dma_ssp_reg(struct ca77xx_audio *audio,
					unsigned int reg)
{
	if (dbg_flag & AUDIO_REG_ACCESS_RD) {
		u32 val = readl(audio->dma_ssp_base + reg);

		dev_dbg(audio->dev, "R, dma-ssp, reg=0x%02x, val=0x%08x\n", reg,
			val);

		return val;
	}

	return readl(audio->dma_ssp_base + reg);
}

static inline void ca77xx_wr_dma_ssp_reg(struct ca77xx_audio *audio,
					 unsigned int reg, u32 val)
{
	if (dbg_flag & AUDIO_REG_ACCESS_WR)
		dev_dbg(audio->dev, "W, dma-ssp, reg=0x%02x, val=0x%08x\n", reg,
			val);

	writel(val, audio->dma_ssp_base + reg);
}

static int match_name(struct device *dev, void *name)
{
	struct platform_device *pdev = to_platform_device(dev);
	char *pdevname = (char *)pdev->name;

	/* pdevname like f7001b80.sspx */
	return !strcmp(pdevname + 9, name);
}

/* Export non ASoC Platform(DMA) APIs *****************************************/
void *ca77xx_snd_audio_buf_req(void *dev_instance, int dir, int block_offset)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	unsigned int diff;
	int total_buf_size;

	if (dir == DIR_TX) {
		diff = ca77xx_snd_audio_dma_curr_offset(audio, DIR_TX);
		total_buf_size = audio->dma_ctrl.tx_buf_num * audio->dma_ctrl.tx_buf_size;
		return audio->dma_ctrl.tx_buf + (diff + audio->dma_ctrl.tx_buf_size * (block_offset + 1)) % total_buf_size;
	} else {
		diff = ca77xx_snd_audio_dma_curr_offset(audio, DIR_RX);
		total_buf_size = audio->dma_ctrl.rx_buf_num * audio->dma_ctrl.rx_buf_size;
		return audio->dma_ctrl.rx_buf + (diff + total_buf_size - audio->dma_ctrl.rx_buf_size * (block_offset + 1)) % total_buf_size;
	}
}
EXPORT_SYMBOL(ca77xx_snd_audio_buf_req);

/* Export ASoC Platform(DMA) APIs *********************************************/
int ca77xx_snd_audio_dma_update(void *dev_instance, int dir, u16 new_idx)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;

	if (dir == DIR_TX)
		return ca77xx_audio_dma_tx_update(audio, new_idx);
	else
		return ca77xx_audio_dma_rx_update(audio, new_idx);

	return 0;
}

int ca77xx_snd_audio_dma_callback(void *dev_instance, int dir,
				  void (*callback)(void *), void *data)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;

	if (dbg_flag & AUDIO_CONFIG)
		dev_dbg(audio->dev, "%s callback 0x%p, data 0x%p\n",
			dir == DIR_TX ? "tx" : "rx", callback, data);

	if (dir == DIR_TX) {
		audio->dma_ctrl.dma_tx_notifier = callback;
		audio->dma_ctrl.tx_data = data;
	} else {
		audio->dma_ctrl.dma_rx_notifier = callback;
		audio->dma_ctrl.rx_data = data;
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_dma_callback);

int ca77xx_snd_audio_dma_register(void *dev_instance, int dir,
				  unsigned int buf_num, unsigned int buf_size)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_DMA_SSP_Q_TXQ_BASE_DEPTH_t reg_txq_base;
	DMA_SEC_DMA_SSP_Q_RXQ_BASE_DEPTH_t reg_rxq_base;
	struct ca77xx_audio_dma_ctrl *dma_ctrl_p;
	struct ca77xx_audio_dma_desc *dma_desc_p;
	int i, power;

	if (dbg_flag & AUDIO_CONFIG)
		dev_dbg(audio->dev, "%s buf_num %d, buf_size %d\n",
			dir == DIR_TX ? "tx" : "rx", buf_num, buf_size);

	dma_ctrl_p = &audio->dma_ctrl;
	power = ca77xx_audio_dma_buf_check(buf_num, buf_size);
	if (power < 0)
		return -EINVAL;

	if (dir == DIR_TX)
		goto DMA_TX_PREPARE;
	else if (dir == DIR_RX)
		goto DMA_RX_PREPARE;
	else
		return -EPERM;

DMA_TX_PREPARE:
	dma_ctrl_p->tx_buf = dma_alloc_coherent(audio->dev, buf_num * buf_size,
				    &dma_ctrl_p->tx_buf_paddr, GFP_KERNEL | GFP_DMA);
	if (!dma_ctrl_p->tx_buf) {
		dev_err(audio->dev, "fail to alloc dma buffer(dir %d, buf_num %d, buf_size %d)\n",
			dir, buf_num, buf_size);
		return -ENOMEM;
	}
	if (!IS_ALIGNED(dma_ctrl_p->tx_buf_paddr, DMA_BUF_ALIGN)) {
		dev_err(audio->dev, "dma tx buffer 0x%p not %d bytes alignment\n",
			dma_ctrl_p->tx_buf_paddr, DMA_BUF_ALIGN);
		return -EFAULT;
	}
	if (dbg_flag & AUDIO_CONFIG)
	dev_dbg(audio->dev, "tx buf: addr 0x%p, paddr %pad, size 0x%x\n",
		dma_ctrl_p->tx_buf, &dma_ctrl_p->tx_buf_paddr,
		buf_num * buf_size);

	dma_ctrl_p->tx_dma_desc = dma_alloc_coherent(audio->dev,
				    buf_num * sizeof(struct ca77xx_audio_dma_desc),
				    &dma_ctrl_p->tx_desc_paddr, GFP_KERNEL | GFP_DMA);
	if (!dma_ctrl_p->tx_dma_desc) {
		dev_err(audio->dev, "fail to alloc dma descriptor(dir %d, buf_num %d)\n",
			dir, buf_num);
		return -ENOMEM;
	}
	if (!IS_ALIGNED(dma_ctrl_p->tx_desc_paddr, DMA_DESC_ALIGN)) {
		dev_err(audio->dev, "dma tx desc 0x%p not %d bytes alignment\n",
			dma_ctrl_p->tx_desc_paddr, DMA_DESC_ALIGN);
		return -EFAULT;
	}
	if (dbg_flag & AUDIO_CONFIG)
	dev_dbg(audio->dev, "tx desc: addr 0x%p, paddr %pad, size 0x%zx\n",
		dma_ctrl_p->tx_dma_desc, &dma_ctrl_p->tx_desc_paddr,
		buf_num * sizeof(struct ca77xx_audio_dma_desc));

	dma_ctrl_p->tx_buf_num = buf_num;
	dma_ctrl_p->tx_buf_size = buf_size;

	for (i = 0; i < buf_num; i++) {
		dma_desc_p = (struct ca77xx_audio_dma_desc *)(dma_ctrl_p->tx_dma_desc + i);
		dma_desc_p->sgm = 1; /* be always enabled to work with flash */
		dma_desc_p->own = OWN_BY_CPU;
		dma_desc_p->buf_size = buf_size;
		dma_desc_p->buf_paddr = dma_ctrl_p->tx_buf_paddr + i * buf_size;
	}

	reg_txq_base.wrd = 0;
	reg_txq_base.bf.base =  dma_ctrl_p->tx_desc_paddr >> 4;
	reg_txq_base.bf.depth = power;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_BASE_DEPTH, reg_txq_base.wrd);

	/* fill with special pattern for internal loopback diagnostic */
	if ((audio->ssp_ctrl.flag & FLAG_INT_LB) || (pattern)) {
		u8 *buf = (u8 *)dma_ctrl_p->tx_buf;
		int i;

		for (i = 0; i < buf_num * buf_size; i++) {
			if ((i % buf_size) == 0)
				*buf = (i / buf_num) % buf_size;
			if ((i % 8) == 4)
				*buf = pattern ? pattern : 0x5A;
			if ((i % 8) == 5)
				*buf = pattern ? ~pattern : 0xA5;
			buf++;
		}
	}

	return 0;

DMA_RX_PREPARE:
	dma_ctrl_p->rx_buf = dma_alloc_coherent(audio->dev, buf_num * buf_size,
				    &dma_ctrl_p->rx_buf_paddr, GFP_KERNEL | GFP_DMA);
	if (!dma_ctrl_p->rx_buf) {
		dev_err(audio->dev, "fail to alloc dma buffer(dir %d, buf_num %d, buf_size %d)\n",
			dir, buf_num, buf_size);
		return -ENOMEM;
	}
	if (!IS_ALIGNED(dma_ctrl_p->rx_buf_paddr, DMA_BUF_ALIGN)) {
		dev_err(audio->dev, "dma rx buffer 0x%p not %d bytes alignment\n",
			dma_ctrl_p->rx_buf_paddr, DMA_BUF_ALIGN);
		return -EFAULT;
	}
	if (dbg_flag & AUDIO_CONFIG)
	dev_dbg(audio->dev, "rx buf: addr 0x%p, paddr %pad, size 0x%x\n",
		dma_ctrl_p->rx_buf, &dma_ctrl_p->rx_buf_paddr,
		buf_num * buf_size);

	dma_ctrl_p->rx_dma_desc = dma_alloc_coherent(audio->dev,
				    buf_num * sizeof(struct ca77xx_audio_dma_desc),
				    &dma_ctrl_p->rx_desc_paddr, GFP_KERNEL | GFP_DMA);
	if (!dma_ctrl_p->rx_dma_desc) {
		dev_err(audio->dev, "fail to alloc dma descriptor(dir %d, buf_num %d)\n",
			dir, buf_num);
		return -ENOMEM;
	}
	if (!IS_ALIGNED(dma_ctrl_p->rx_desc_paddr, DMA_DESC_ALIGN)) {
		dev_err(audio->dev, "dma rx desc 0x%p not %d bytes alignment\n",
			dma_ctrl_p->rx_desc_paddr, DMA_DESC_ALIGN);
		return -EFAULT;
	}
	if (dbg_flag & AUDIO_CONFIG)
	dev_dbg(audio->dev, "rx desc: addr 0x%p, paddr %pad, size 0x%zx\n",
		dma_ctrl_p->rx_dma_desc, &dma_ctrl_p->rx_desc_paddr,
		buf_num * sizeof(struct ca77xx_audio_dma_desc));

	dma_ctrl_p->rx_buf_num = buf_num;
	dma_ctrl_p->rx_buf_size = buf_size;

	for (i = 0; i < buf_num; i++) {
		dma_desc_p = (struct ca77xx_audio_dma_desc *)(dma_ctrl_p->rx_dma_desc + i);
		dma_desc_p->own = OWN_BY_DMA;
		dma_desc_p->buf_size = buf_size;
		dma_desc_p->buf_paddr = dma_ctrl_p->rx_buf_paddr + i * buf_size;
	}

	reg_rxq_base.wrd = 0;
	reg_rxq_base.bf.base =  dma_ctrl_p->rx_desc_paddr >> 4;
	reg_rxq_base.bf.depth = power;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_BASE_DEPTH, reg_rxq_base.wrd);

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_dma_register);

int ca77xx_snd_audio_dma_unregister(void *dev_instance, int dir)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	struct ca77xx_audio_dma_ctrl *dma_ctrl_p = &audio->dma_ctrl;
	size_t size;

	if (dir == DIR_TX) {
		if (dma_ctrl_p->tx_buf) {
			size = dma_ctrl_p->tx_buf_num * dma_ctrl_p->tx_buf_size;
			dma_free_coherent(audio->dev, size, dma_ctrl_p->tx_buf,
					  dma_ctrl_p->tx_buf_paddr);
			dma_ctrl_p->tx_buf = NULL;
		}

		if (dma_ctrl_p->tx_dma_desc) {
			size = dma_ctrl_p->tx_buf_num * sizeof(struct ca77xx_audio_dma_desc);
			dma_free_coherent(audio->dev, size, dma_ctrl_p->tx_dma_desc,
					  dma_ctrl_p->tx_desc_paddr);
			dma_ctrl_p->tx_dma_desc = NULL;
		}
	}

	if (dir == DIR_RX) {
		if (dma_ctrl_p->rx_buf) {
			size = dma_ctrl_p->rx_buf_num * dma_ctrl_p->rx_buf_size;
			dma_free_coherent(audio->dev, size, dma_ctrl_p->rx_buf,
					  dma_ctrl_p->rx_buf_paddr);
			dma_ctrl_p->rx_buf = NULL;
		}

		if (dma_ctrl_p->rx_dma_desc) {
			size = dma_ctrl_p->rx_buf_num * sizeof(struct ca77xx_audio_dma_desc);
			dma_free_coherent(audio->dev, size, dma_ctrl_p->rx_dma_desc,
					  dma_ctrl_p->rx_desc_paddr);
			dma_ctrl_p->rx_dma_desc = NULL;
		}
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_dma_unregister);

int ca77xx_snd_audio_start(void *dev_instance, int dir)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_SSP_CTRL0_t reg_ssp_ctrl0;
	DMA_SEC_SSP_CTRL1_t reg_ssp_ctrl1;
	unsigned long flags;

	if ((audio->ssp_ctrl.type == CA77XX_AUDIO_PCM) && (dir == DIR_TX)) {
		audio->dma_ctrl.tx_write_ptr = 2;
		ca77xx_audio_dma_tx_update(audio, audio->dma_ctrl.tx_write_ptr);
	}
	if ((audio->ssp_ctrl.type == CA77XX_AUDIO_PCM) && (dir == DIR_RX)) {
		audio->dma_ctrl.rx_read_ptr = 0;
		ca77xx_audio_dma_rx_update(audio, audio->dma_ctrl.rx_read_ptr);
	}

	if (dir == DIR_TX) {
		ca77xx_audio_dma_tx_enable(audio);

		spin_lock_irqsave(&audio->lock, flags);
		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
		if (audio->ssp_ctrl.slave_fs)
			reg_ssp_ctrl0.bf.pcm_slv_en = 1;
		reg_ssp_ctrl0.bf.forceTC = 0;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);

		if (audio->ssp_ctrl.flag & FLAG_INT_LB) {
			reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
			reg_ssp_ctrl0.bf.loop_en = 1;
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
		} else {
			reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
			reg_ssp_ctrl0.bf.loop_en = 0;
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
		}

		reg_ssp_ctrl1.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL1);
		reg_ssp_ctrl1.bf.startProc = 1;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL1, reg_ssp_ctrl1.wrd);

		spin_unlock_irqrestore(&audio->lock, flags);
	} else {
		ca77xx_audio_dma_rx_enable(audio);
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_start);

int ca77xx_snd_audio_stop(void *dev_instance, int dir)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_SSP_CTRL0_t reg_ssp_ctrl0;
	DMA_SEC_SSP_FIFO_PTR_t reg_ssp_fifo_ptr;
	unsigned long flags;
	int cnt;

	if (dir == DIR_TX) {
		/* disable SSP */
		spin_lock_irqsave(&audio->lock, flags);

		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
		if (audio->ssp_ctrl.slave_fs)
			reg_ssp_ctrl0.bf.pcm_slv_en = 0;
		reg_ssp_ctrl0.bf.forceTC = 1;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);

		usleep_range(1000, 2000);
		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
		reg_ssp_ctrl0.bf.forceTC = 0;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
		spin_unlock_irqrestore(&audio->lock, flags);

		mdelay(20);

		/* disable DMA */
		cnt = 0;
		do {
			reg_ssp_fifo_ptr.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FIFO_PTR);

			if (reg_ssp_fifo_ptr.bf.tfWrPtr == reg_ssp_fifo_ptr.bf.tfRdPtr)
				break;

			cnt++;
			mdelay(1);
		} while (cnt <= 50);

		if (cnt > 50)
			dev_warn(audio->dev, "Func: %s - forceTc to Tx idle timeout\n", __func__);

		mdelay(1);

		ca77xx_audio_dma_tx_disable(audio);
	} else {
		cnt = 0;
		do {
			reg_ssp_fifo_ptr.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FIFO_PTR);

			if (reg_ssp_fifo_ptr.bf.rfWrPtr == reg_ssp_fifo_ptr.bf.rfRdPtr)
				break;

			cnt++;
			mdelay(1);
		} while (cnt <= 50);

		if (cnt > 50)
			dev_warn(audio->dev, "Func: %s - forceTc to Rx idle timeout\n", __func__);

		ca77xx_audio_dma_rx_disable(audio);
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_stop);

unsigned int ca77xx_snd_audio_dma_curr_offset(void *dev_instance, int dir)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	u16 wt_idx, rd_idx;
	unsigned int offset;

	if (dir == DIR_TX) {
		ca77xx_audio_dma_tx_idx(audio, &wt_idx, &rd_idx);
		offset = rd_idx * audio->dma_ctrl.tx_buf_size;
	} else {
		ca77xx_audio_dma_rx_idx(audio, &wt_idx, &rd_idx);
		offset = wt_idx * audio->dma_ctrl.rx_buf_size;
	}

	return offset;
}

/* Export ASoC DAI APIs *******************************************************/
int ca77xx_snd_audio_clkdiv(void *dev_instance, int clk_src, int data_rate)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_SSP_FRAME_CTRL0_t reg_frame_ctrl0;
	unsigned long sclk_hz, clk_hz, cdiv;

	printk("%s\n", __func__);

	if (dbg_flag & AUDIO_CONFIG)
		dev_dbg(audio->dev, "clk_src %d, data_rate %d Hz\n", clk_src,
			data_rate);

	switch (clk_src) {
	case AUDIO_INT_SYS_CLK_DIV:
		if (data_rate != 0) {
			clk_hz = clk_get_rate(audio->clk);
			sclk_hz = data_rate;

			cdiv = DIV_ROUND_UP(clk_hz, sclk_hz);
			cdiv += (cdiv % 2);

			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, cdiv / 2);
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, 0);
		} else {
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, 0);
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, 0);
		}
		break;

	case AUDIO_EXT_ISI_CLK_DIV:
		if (data_rate != 2048000 && data_rate != 0)
			return -EINVAL;

		if (data_rate != 0) {
			reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
			reg_frame_ctrl0.bf.extClkSel = 1;
			reg_frame_ctrl0.wrd |= BIT(27); /* ISI only */
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

			/* FPGA only ? */
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, 0X2AAAAAAA);
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, 0XFFFFFFFC);
		} else {
			reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
			reg_frame_ctrl0.bf.extClkSel = 0;
			reg_frame_ctrl0.wrd &= ~BIT(27);
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

			/* FPGA only ? */
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, 0);
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, 0);
		}
		break;

	case AUDIO_EXT_ZSI_CLK_DIV:
		if (data_rate != 2048000 && data_rate != 0)
			return -EINVAL;

		if (data_rate != 0) {
			reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
			reg_frame_ctrl0.bf.extClkSel = 1;
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

			/* FPGA only ? */
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, 0X15555555);
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, 0XFFFFFFFC);
		} else {
			reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
			reg_frame_ctrl0.bf.extClkSel = 0;
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

			/* FPGA only ? */
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, 0);
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, 0);
		}
		break;

	default:
		dev_err(audio->dev, "invalid clk_src(%d) selection!\n", clk_src);
		return -EPERM;
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_clkdiv);

int ca77xx_snd_audio_clkcfg(void *dev_instance, int data_rate)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_SSP_FRAME_CTRL0_t reg_frame_ctrl0;
	unsigned long sclk_hz, clk_hz, cdiv;
	u32 dividend, sumValue, baud_cnt_upbond;

	if (dbg_flag & AUDIO_CONFIG)
		dev_dbg(audio->dev, "data rate %d Hz\n", data_rate);

	if (((data_rate % 8000) != 0) && ((data_rate % 11025) != 0))
		return -EINVAL;

	if (audio->ssp_ctrl.ext_clk == 1) {
		/* external reference clock */
		if (data_rate == 0) {
			reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
			reg_frame_ctrl0.bf.extClkSel = 0;
			reg_frame_ctrl0.bf.extSClkSel = 0;
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, 0);
			if (audio->ssp_ctrl.clk_enhance)
				ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, 0);

		} else if (clk_get_rate(audio->clk) == data_rate) {
			/* use directly, phase and rate must be correct */
			reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
			reg_frame_ctrl0.bf.extSClkSel = 1;
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

		} else {
			clk_hz = clk_get_rate(audio->clk);
			sclk_hz = data_rate;

			cdiv = DIV_ROUND_UP(clk_hz, sclk_hz);
			cdiv += (cdiv % 2);

			dividend = CLK_UPBOUND;
			sumValue = dividend / (cdiv / 2);
			baud_cnt_upbond = sumValue * (cdiv / 2);

			/*
			 * ISI: sumValue = 0X2AAAAAAA, baud_cnt_upbond = 0XFFFFFFFC
			 * ZSI: sumValue = 0X15555555, baud_cnt_upbond = 0XFFFFFFFC
			 */

			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, sumValue);
			if (audio->ssp_ctrl.clk_enhance)
				ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, baud_cnt_upbond);

			reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
			reg_frame_ctrl0.bf.extClkSel = 1;
			if (audio->ssp_ctrl.clk_enhance && audio->ssp_ctrl.clk_src_pol)
				reg_frame_ctrl0.wrd |= BIT(27);
			else
				reg_frame_ctrl0.wrd &= ~BIT(27);
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);
		}
	} else {
		/* internal reference clock */
		if (data_rate == 0) {
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, 0);
			if (audio->ssp_ctrl.clk_enhance)
				ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, 0);

		} else {
			clk_hz = clk_get_rate(audio->clk);
			sclk_hz = data_rate;

			cdiv = DIV_ROUND_UP(clk_hz, sclk_hz);
			cdiv += (cdiv % 2);

			dividend = CLK_UPBOUND;
			sumValue = dividend / (cdiv / 2);
			baud_cnt_upbond = sumValue * (cdiv / 2);

			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL, sumValue);
			if (audio->ssp_ctrl.clk_enhance)
				ca77xx_wr_ssp_reg(audio, CA77XX_SSP_BAUDRATE_CTRL1, baud_cnt_upbond);
		}
	}

	return 0;
}
#ifndef G3_TEST
EXPORT_SYMBOL(ca77xx_snd_audio_clkcfg);
#endif

int ca77xx_snd_audio_slots(void *dev_instance, unsigned int tx_mask,
			   unsigned int rx_mask, int slots, int width)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_SSP_FRAME_CTRL1_t reg_frame_ctrl1;
	u32 reg;
	int i;

	if (dbg_flag & AUDIO_CONFIG){
#ifdef G3_TEST
		dev_dbg(audio->dev, "tx_mask 0x%08x, rx_mask 0x%08x, slots %d, width %d\n",
			tx_mask, rx_mask, slots, width);
#else
		dev_dbg(audio->dev, "tx_mask 0x%08x, rx_mask 0x%08x, slots %d, width %d\n",
			tx_mask[0], rx_mask[0], slots, width);
#endif
	}
	
#ifndef G3_TEST
	if ((slots <= 0) || (slots > MAX_SLOT_NUM))
		return -EINVAL;
#endif

#ifdef G3_TEST
	ca77xx_wr_ssp_reg(audio, CA77XX_SSP_TXSLOT_VLD0, tx_mask);
	ca77xx_wr_ssp_reg(audio, CA77XX_SSP_RXSLOT_VLD0, rx_mask);
#endif
	reg_frame_ctrl1.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL1);
	reg_frame_ctrl1.bf.numOfSlots = slots - 1;
	ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL1, reg_frame_ctrl1.wrd);
#ifndef G3_TEST
	for (i = 0; i < div_up(slots, 32); i++) {
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_TXSLOT_VLD0 + i * 4, tx_mask[i]);
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_RXSLOT_VLD0 + i * 4, rx_mask[i]);
	}
#endif
	if (slots > 32)
		slots = 32;

	for (i = 0; i < slots; i++) {
		if ((i % 8) == 0)
			reg = 0;

		reg |= (width - 1) << ((i % 8) * 4);

		if (((i + 1) % 8) == 0)
			ca77xx_wr_ssp_reg(audio, CA77XX_SSP_SLOT_SIZE0 + (i / 8) * 4, reg);
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_slots);

int ca77xx_snd_audio_signal_formt(void *dev_instance, unsigned int format)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_SSP_CTRL0_t reg_ssp_ctrl0;
	DMA_SEC_SSP_FRAME_CTRL0_t reg_frame_ctrl0;
	DMA_SEC_SSP_FRAME_CTRL1_t reg_frame_ctrl1;

	if (dbg_flag & AUDIO_CONFIG)
		dev_dbg(audio->dev, "signal format 0x%08x\n", format);

	if (audio->ssp_ctrl.flag & FLAG_ISI) {
		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
		reg_ssp_ctrl0.bf.spi_sel = audio->ssp_ctrl.spi_cs + 1;
		reg_ssp_ctrl0.bf.isi_en = 1;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
	} else if (audio->ssp_ctrl.flag & FLAG_ZSI) {
		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
		reg_ssp_ctrl0.bf.spi_sel = audio->ssp_ctrl.spi_cs + 1;
		reg_ssp_ctrl0.bf.isi_en = 1;
		reg_ssp_ctrl0.bf.zsi_sel_en = 1;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
	}

	switch (format & AUDIO_DATA_FORMAT_MASK) {
	case AUDIO_DATA_I2S:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.FSPol = 0;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

		reg_frame_ctrl1.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL1);
		reg_frame_ctrl1.bf.FSLen = 31;
		reg_frame_ctrl1.bf.FS2Slot = 1;
		reg_frame_ctrl1.bf.slot2FS = 7;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL1, reg_frame_ctrl1.wrd);
		break;
	case AUDIO_DATA_RIGHT_J:
	case AUDIO_DATA_LEFT_J:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.FSPol = 0;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

		reg_frame_ctrl1.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL1);
		reg_frame_ctrl1.bf.FSLen = 31;
		reg_frame_ctrl1.bf.FS2Slot = 0;
		reg_frame_ctrl1.bf.slot2FS = 8;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL1, reg_frame_ctrl1.wrd);
		break;
	case AUDIO_DATA_DSP_A:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.FSPol = 0;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);

		reg_frame_ctrl1.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL1);
		reg_frame_ctrl1.bf.FSLen = 7;
		reg_frame_ctrl1.bf.FS2Slot = 1;
		reg_frame_ctrl1.bf.slot2FS = 7;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL1, reg_frame_ctrl1.wrd);
		break;
	case AUDIO_DATA_DSP_B:
	case AUDIO_DATA_AC97:
	case AUDIO_DATA_PDM:
	default:
		return -1;
	}

	switch (format & AUDIO_CLOCK_GATE_MASK) {
	case AUDIO_CLOCK_CONT:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.FSCFreeRun = 1;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);
		break;
	case AUDIO_CLOCK_GATED:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.FSCFreeRun = 0;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);
		break;
	default:
		return -1;
	}

	switch (format & AUDIO_SIGNAL_INV_MASK) {
	case AUDIO_SIGNAL_NB_NF:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.txEdge = 1;
		reg_frame_ctrl0.bf.rxEdge = 0;
		reg_frame_ctrl0.bf.FSEdge = 1;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);
		break;
	case AUDIO_SIGNAL_NB_IF:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.txEdge = 1;
		reg_frame_ctrl0.bf.rxEdge = 0;
		reg_frame_ctrl0.bf.FSEdge = 0;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);
		break;
	case AUDIO_SIGNAL_IB_NF:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.txEdge = 0;
		reg_frame_ctrl0.bf.rxEdge = 1;
		reg_frame_ctrl0.bf.FSEdge = 1;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);
		break;
	case AUDIO_SIGNAL_IB_IF:
		reg_frame_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0);
		reg_frame_ctrl0.bf.txEdge = 0;
		reg_frame_ctrl0.bf.rxEdge = 1;
		reg_frame_ctrl0.bf.FSEdge = 0;
		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FRAME_CTRL0, reg_frame_ctrl0.wrd);
		break;
	default:
		return -1;
	}

	switch (format & AUDIO_CLOCK_MASTER_MASK) {
	case AUDIO_CLOCK_CBM_CFM:
	case AUDIO_CLOCK_CBS_CFM:
		audio->ssp_ctrl.slave_fs = 0;
//		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
//		reg_ssp_ctrl0.bf.pcm_slv_en = 0;
//		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
		break;
	case AUDIO_CLOCK_CBS_CFS:
	case AUDIO_CLOCK_CBM_CFS:
		audio->ssp_ctrl.slave_fs = 1;
//		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);
//		reg_ssp_ctrl0.bf.pcm_slv_en = 1;
//		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
		break;
	default:
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_signal_formt);

int ca77xx_snd_audio_data_formt(void *dev_instance, int dir, int wide, int endian)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_SSP_CTRL0_t reg_ssp_ctrl0;

	if (dbg_flag & AUDIO_CONFIG)
		dev_dbg(audio->dev, "%s data format wide %d, endian %d\n",
			dir == DIR_TX ? "tx" : "rx", wide, endian);

	if (dir & DIR_TX) {
		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);

		if ((wide == 2) && (endian == 0))
			reg_ssp_ctrl0.bf.txChByteSwap = 1;
		else
			reg_ssp_ctrl0.bf.txChByteSwap = 0;

		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
	} else {
		reg_ssp_ctrl0.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_CTRL0);

		if ((wide == 2) && (endian == 0))
			reg_ssp_ctrl0.bf.rxChByteSwap = 1;
		else
			reg_ssp_ctrl0.bf.rxChByteSwap = 0;

		ca77xx_wr_ssp_reg(audio, CA77XX_SSP_CTRL0, reg_ssp_ctrl0.wrd);
	}

	return 0;
}
#ifndef G3_TEST
EXPORT_SYMBOL(ca77xx_snd_audio_data_formt);
#endif

void *ca77xx_snd_audio_if_register(struct ca77xx_audio_cfg *cfg)
{
	struct platform_device *pdev;
	struct ca77xx_audio *audio;
	struct device *dev;
	char name[32];

	snprintf(name, sizeof(name), "ssp%d", cfg->idx);
	dev = bus_find_device(&platform_bus_type, dev, name, match_name);
	if (!dev) {
		printk(KERN_ERR "%s(): failed to find device for ssp %d\n",
		       __func__, cfg->idx);
		return NULL;
	}
	pdev = to_platform_device(dev);
	audio = platform_get_drvdata(pdev);

	if (dbg_flag & AUDIO_CONFIG)
		dev_dbg(audio->dev, "cfg - idx %d, type %d, dir %d, flag 0x%x, spi_cs %d\n",
			cfg->idx, cfg->type, cfg->dir, cfg->flag, cfg->spi_cs);

	if (audio->ssp_ctrl.type != CA77XX_AUDIO_UNDEF) {
		dev_err(audio->dev, "ssp %d has been registered!\n", cfg->idx);
		return NULL;
	}

	if (cfg->type != CA77XX_AUDIO_PCM && cfg->type != CA77XX_AUDIO_I2S) {
		dev_err(audio->dev, "doesn't support type %d!\n", cfg->type);
		return NULL;
	}

	if (!(cfg->dir & DIR_TX || cfg->dir & DIR_RX)) {
		dev_err(audio->dev, "invalid dir %d!\n", cfg->dir);
		return NULL;
	}

	if ((cfg->type == CA77XX_AUDIO_I2S) &&
	    (cfg->flag & FLAG_ISI || cfg->flag & FLAG_ZSI)) {
		dev_warn(audio->dev, "clear flag 0x%lx!\n", FLAG_ISI | FLAG_ZSI);
		cfg->flag &= ~(FLAG_ISI | FLAG_ZSI);
	}

	if (cfg->spi_cs > MAX_SPI_CS) {
		dev_err(audio->dev, "invalid spi_cs is %d!\n", cfg->spi_cs);
		return NULL;
	}

	audio->ssp_ctrl.type = cfg->type;
	audio->ssp_ctrl.flag = cfg->flag & FLAG_MASK;
	audio->ssp_ctrl.spi_cs = cfg->spi_cs;
#if 1//CONFIG_ARCH_CORTINA_G3
	if (cfg->flag != 0)
#else
	if (cfg->flag & FLAG_ISI)
#endif		
		audio->ssp_ctrl.clk_src_pol = 1;
	else
		audio->ssp_ctrl.clk_src_pol = 0;

	return (void *)audio;
}
EXPORT_SYMBOL(ca77xx_snd_audio_if_register);

int ca77xx_snd_audio_if_unregister(void *dev_instance)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;

	audio->ssp_ctrl.type = 0;
	audio->ssp_ctrl.flag = 0;
	audio->ssp_ctrl.spi_cs = 0;

	return 0;
}
EXPORT_SYMBOL(ca77xx_snd_audio_if_unregister);

/* Audio Interval Data and Functions ******************************************/
static int ca77xx_audio_dma_tx_idx(struct ca77xx_audio *audio, u16 *wt_idx_p,
				   u16 *rd_idx_p)
{
	DMA_SEC_DMA_SSP_Q_TXQ_WPTR_t reg_txq_wptr;
	DMA_SEC_DMA_SSP_Q_TXQ_RPTR_t reg_txq_rptr;

	reg_txq_wptr.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_WPTR);
	*wt_idx_p = reg_txq_wptr.bf.index;

	reg_txq_rptr.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_RPTR);
	*rd_idx_p = reg_txq_rptr.bf.index;

	return 0;
}

static int ca77xx_audio_dma_rx_idx(struct ca77xx_audio *audio, u16 *wt_idx_p,
				   u16 *rd_idx_p)
{
	DMA_SEC_DMA_SSP_Q_RXQ_WPTR_t reg_rxq_wptr;
	DMA_SEC_DMA_SSP_Q_RXQ_RPTR_t reg_rxq_rptr;

	reg_rxq_wptr.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_WPTR);
	*wt_idx_p = reg_rxq_wptr.bf.index;

	reg_rxq_rptr.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_RPTR);
	*rd_idx_p = reg_rxq_rptr.bf.index;

	return 0;
}

static int ca77xx_audio_dma_tx_update(struct ca77xx_audio *audio,
				      u16 new_wt_idx)
{
	struct ca77xx_audio_dma_ctrl *dma_ctrl_p;
	u16 wt_idx, rd_idx;
	int change = 0;

	dma_ctrl_p = &audio->dma_ctrl;

	ca77xx_audio_dma_tx_idx(audio, &wt_idx, &rd_idx);

	/* update buffer descriptor own bit */
	while (wt_idx != new_wt_idx) {
#ifndef BUG48545
		if (dma_ctrl_p->tx_dma_desc[wt_idx].own == OWN_BY_DMA) {
			dev_err(audio->dev, "Can't Update SSP%d Tx-(w:%d,r:%d) to %d\n",
				idx, wt_idx, rd_idx, new_wt_idx);
			break;
		}
#endif
		change = 1;

		dma_ctrl_p->tx_dma_desc[wt_idx].own = OWN_BY_DMA;
		dma_ctrl_p->tx_dma_desc[wt_idx].buf_size = dma_ctrl_p->tx_buf_size;
		wt_idx = (wt_idx + 1) % dma_ctrl_p->tx_buf_num;
	}

	if (change == 1)
		ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_WPTR, wt_idx);

	return 0;
}

static int ca77xx_audio_dma_rx_update(struct ca77xx_audio *audio,
				      u16 new_rd_idx)
{
	struct ca77xx_audio_dma_ctrl *dma_ctrl_p;
	u16 wt_idx, rd_idx;
	int change = 0;

	dma_ctrl_p = &audio->dma_ctrl;

	ca77xx_audio_dma_rx_idx(audio, &wt_idx, &rd_idx);

	/* update buffer descriptor own bit */
	while (rd_idx != new_rd_idx) {
#ifndef BUG48545
		if (dma_ctrl_p->rx_dma_desc[rd_idx].own == OWN_BY_DMA) {
			dev_err(audio->dev, "Can't Update SSP%d Rx-(w:%d,r:%d) to %d\n",
				idx, wt_idx, rd_idx, new_rd_idx);
			break;
		}
#endif
		change = 1;

		dma_ctrl_p->rx_dma_desc[rd_idx].own = OWN_BY_DMA;
		dma_ctrl_p->rx_dma_desc[rd_idx].buf_size = dma_ctrl_p->rx_buf_size;
		rd_idx = (rd_idx + 1) % dma_ctrl_p->rx_buf_num;
	}

	if (change == 1)
		ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_RPTR, rd_idx);

	return 0;
}

static int ca77xx_audio_dma_tx_enable(struct ca77xx_audio *audio)
{
	DMA_SEC_DMA_SSP_Q_TXQ_CONTROL_t reg_txq_ctrl;
	DMA_SEC_DMA_SSP_Q_TXQ_INTENABLE_t reg_txq_inten;
	DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTENABLE_t reg_txq_coal_inten;
	DMA_SEC_DMA_SSP_Q_TXQ_MISC_INTENABLE_t reg_txq_misc_inten;
	unsigned long flags;

	ca77xx_audio_dma_cnt_init(audio, DIR_TX);

	spin_lock_irqsave(&audio->lock, flags);

	reg_txq_ctrl.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_CONTROL);
	reg_txq_ctrl.bf.burst_size = 0;	/* 128 bytes */
	reg_txq_ctrl.bf.txq_flush_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_CONTROL, reg_txq_ctrl.wrd);

	reg_txq_ctrl.bf.txq_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_CONTROL, reg_txq_ctrl.wrd);

	reg_txq_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_INTENABLE);
	reg_txq_inten.bf.TXQ_COAL_INTERRUPTi_en = 1;
	reg_txq_inten.bf.TXQ_MISC_INTERRUPTi_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_INTENABLE, reg_txq_inten.wrd);

	reg_txq_coal_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_COAL_INTENABLE);
	reg_txq_coal_inten.bf.txq_eof_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_COAL_INTENABLE, reg_txq_coal_inten.wrd);

	reg_txq_misc_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_MISC_INTENABLE);
	reg_txq_misc_inten.bf.txq_desc_err_en = 1;
	reg_txq_misc_inten.bf.txq_overrun_en = 1;
	reg_txq_misc_inten.bf.txq_empty_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_MISC_INTENABLE, reg_txq_misc_inten.wrd);

	spin_unlock_irqrestore(&audio->lock, flags);

	return 0;
}

static int ca77xx_audio_dma_rx_enable(struct ca77xx_audio *audio)
{
	DMA_SEC_DMA_SSP_Q_RXQ_CONTROL_t reg_rxq_ctrl;
	DMA_SEC_DMA_SSP_Q_RXQ_INTENABLE_t reg_rxq_inten;
	DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTENABLE_t reg_rxq_coal_inten;
	DMA_SEC_DMA_SSP_Q_RXQ_MISC_INTENABLE_t reg_rxq_misc_inten;
	unsigned long flags;

	ca77xx_audio_dma_cnt_init(audio, DIR_RX);

	spin_lock_irqsave(&audio->lock, flags);

	reg_rxq_ctrl.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_CONTROL);
	reg_rxq_ctrl.bf.burst_size = 0;	/* 128 bytes */
	reg_rxq_ctrl.bf.rxq_flush_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_CONTROL, reg_rxq_ctrl.wrd);

	reg_rxq_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_INTENABLE);
	reg_rxq_inten.bf.RXQ_COAL_INTERRUPTi_en = 1;
	reg_rxq_inten.bf.RXQ_MISC_INTERRUPTi_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_INTENABLE, reg_rxq_inten.wrd);

	reg_rxq_coal_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_COAL_INTENABLE);
	reg_rxq_coal_inten.bf.rxq_eof_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_COAL_INTENABLE, reg_rxq_coal_inten.wrd);

	reg_rxq_misc_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_MISC_INTENABLE);
	reg_rxq_misc_inten.bf.rxq_desc_err_en = 1;
	reg_rxq_misc_inten.bf.rxq_overrun_en = 1;
	reg_rxq_misc_inten.bf.rxq_full_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_MISC_INTENABLE, reg_rxq_misc_inten.wrd);

	spin_unlock_irqrestore(&audio->lock, flags);

	return 0;
}

static int ca77xx_audio_dma_tx_disable(struct ca77xx_audio *audio)
{
	DMA_SEC_DMA_SSP_Q_TXQ_CONTROL_t reg_txq_ctrl;
	DMA_SEC_DMA_SSP_Q_TXQ_INTENABLE_t reg_txq_inten;
	DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTENABLE_t reg_txq_coal_inten;
	DMA_SEC_DMA_SSP_Q_TXQ_MISC_INTENABLE_t reg_txq_misc_inten;
	struct ca77xx_audio_dma_desc *dma_desc_p;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&audio->lock, flags);

	reg_txq_ctrl.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_CONTROL);
	reg_txq_ctrl.bf.txq_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_CONTROL, reg_txq_ctrl.wrd);

	reg_txq_ctrl.bf.txq_flush_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_CONTROL, reg_txq_ctrl.wrd);

	/* software should reset write pointer, hardware only reset read pointer */
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_WPTR, 0);

	/* disable interrupts */
	reg_txq_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_INTENABLE);
	reg_txq_inten.bf.TXQ_COAL_INTERRUPTi_en = 0;
	reg_txq_inten.bf.TXQ_MISC_INTERRUPTi_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_INTENABLE, reg_txq_inten.wrd);

	reg_txq_coal_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_COAL_INTENABLE);
	reg_txq_coal_inten.bf.txq_eof_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_COAL_INTENABLE, reg_txq_coal_inten.wrd);

	reg_txq_misc_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_MISC_INTENABLE);
	reg_txq_misc_inten.bf.txq_desc_err_en = 0;
	reg_txq_misc_inten.bf.txq_overrun_en = 0;
	reg_txq_misc_inten.bf.txq_empty_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_MISC_INTENABLE, reg_txq_misc_inten.wrd);
	reg_txq_misc_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_MISC_INTENABLE);

	/* reset DMA descriptor */
	for (i = 0; i < audio->dma_ctrl.tx_buf_num; i++) {
		dma_desc_p = (struct ca77xx_audio_dma_desc *)(audio->dma_ctrl.tx_dma_desc + i);
		dma_desc_p->own = OWN_BY_CPU;
		dma_desc_p->buf_size = audio->dma_ctrl.tx_buf_size;
		/* dma_desc_p->buf_paddr = dma_ctrl_p->tx_buf_paddr + i * buf_size; */
	}

	spin_unlock_irqrestore(&audio->lock, flags);

	ca77xx_audio_dma_cnt_disp(audio, DIR_TX);

	return 0;
}

static int ca77xx_audio_dma_rx_disable(struct ca77xx_audio *audio)
{
	DMA_SEC_DMA_SSP_Q_RXQ_INTENABLE_t reg_rxq_inten;
	DMA_SEC_DMA_SSP_Q_RXQ_CONTROL_t reg_rxq_ctrl;
	DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTENABLE_t reg_rxq_coal_inten;
	DMA_SEC_DMA_SSP_Q_RXQ_MISC_INTENABLE_t reg_rxq_misc_inten;
	struct ca77xx_audio_dma_desc *dma_desc_p;
	u16 wt_idx, rd_idx;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&audio->lock, flags);

	/* flush left data */
	ca77xx_audio_dma_rx_idx(audio, &wt_idx, &rd_idx);
	ca77xx_audio_dma_rx_update(audio, wt_idx);

	reg_rxq_ctrl.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_CONTROL);
	reg_rxq_ctrl.bf.rxq_flush_en = 1;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_CONTROL, reg_rxq_ctrl.wrd);

	/* software should reset read pointer, hardware only reset write pointer */
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_RPTR, 0);

	reg_rxq_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_INTENABLE);
	reg_rxq_inten.bf.RXQ_COAL_INTERRUPTi_en = 0;
	reg_rxq_inten.bf.RXQ_MISC_INTERRUPTi_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_INTENABLE, reg_rxq_inten.wrd);

	reg_rxq_coal_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_COAL_INTENABLE);
	reg_rxq_coal_inten.bf.rxq_eof_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_COAL_INTENABLE, reg_rxq_coal_inten.wrd);

	reg_rxq_misc_inten.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_MISC_INTENABLE);
	reg_rxq_misc_inten.bf.rxq_desc_err_en = 0;
	reg_rxq_misc_inten.bf.rxq_overrun_en = 0;
	reg_rxq_misc_inten.bf.rxq_full_en = 0;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_MISC_INTENABLE, reg_rxq_misc_inten.wrd);

	/* reset DMA descriptor */
	for (i = 0; i < audio->dma_ctrl.rx_buf_num; i++) {
		dma_desc_p = (struct ca77xx_audio_dma_desc *)(audio->dma_ctrl.rx_dma_desc + i);
		dma_desc_p->own = OWN_BY_DMA;
		dma_desc_p->buf_size = audio->dma_ctrl.rx_buf_size;
		/* dma_desc_p->buf_paddr = dma_ctrl_p->rx_buf_paddr + i * buf_size; */
	}

	spin_unlock_irqrestore(&audio->lock, flags);

	ca77xx_audio_dma_cnt_disp(audio, DIR_RX);

	return 0;
}

static int ca77xx_audio_dma_buf_check(u32 num, u32 size)
{
	int i;

	if ((num == 0) || (size == 0))
		return -EINVAL;

	if ((size % 128) != 0)
		return -EINVAL;

	if (powerof2(num) == false)
		return -EINVAL;

	for (i = CA77XX_DMA_SSP_DEPTH_MIN; i <= CA77XX_DMA_SSP_DEPTH_MAX; i++)
		if (num == (0x0001 << i))
			return i;

	return -EINVAL;
}

static void ca77xx_audio_dma_cnt_init(struct ca77xx_audio *audio, int dir)
{
	/* TBD */
}

static void ca77xx_audio_dma_cnt_disp(struct ca77xx_audio *audio, int dir)
{
	/* TBD */
}

static void ca77xx_audio_dma_cnt_update(struct ca77xx_audio *audio, int dir, u32 reg)
{
	/* TBD */
}

static irqreturn_t ca77xx_audio_dma_tx_irq_handler(int irq, void *dev_instance)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_DMA_SSP_Q_TXQ_COAL_INTERRUPT_t reg_txq_int;
	DMA_SEC_DMA_SSP_Q_TXQ_MISC_INTERRUPT_t reg_txq_misc_int;
	u32 reg;
	u32 eof = 0;

	reg_txq_int.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_COAL_INTERRUPT);
	reg = reg_txq_int.wrd;
	eof = reg_txq_int.bf.txq_eof;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_COAL_INTERRUPT, reg_txq_int.wrd);

	reg_txq_misc_int.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_MISC_INTERRUPT);
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_MISC_INTERRUPT, reg_txq_misc_int.wrd);
	if (reg_txq_misc_int.wrd != 0)
		dev_warn(audio->dev, "reg_txq_misc_int.wrd=0x%x\n", reg_txq_misc_int.wrd);

	ca77xx_audio_dma_cnt_update(audio, DIR_TX, reg);

	if (eof) {
		if (audio->ssp_ctrl.type == CA77XX_AUDIO_PCM) {
			audio->dma_ctrl.tx_write_ptr = (audio->dma_ctrl.tx_write_ptr + 1) % audio->dma_ctrl.tx_buf_num;
			ca77xx_audio_dma_tx_update(audio, audio->dma_ctrl.tx_write_ptr);
		}

		if (audio->dma_ctrl.dma_tx_notifier)
			audio->dma_ctrl.dma_tx_notifier(audio->dma_ctrl.tx_data);
	}

	return IRQ_HANDLED;
}

static irqreturn_t ca77xx_audio_dma_rx_irq_handler(int irq, void *dev_instance)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	DMA_SEC_DMA_SSP_Q_RXQ_COAL_INTERRUPT_t reg_rxq_int;
	DMA_SEC_DMA_SSP_Q_RXQ_MISC_INTERRUPT_t reg_rxq_misc_int;
	u32 reg, eof;

	reg_rxq_int.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_COAL_INTERRUPT);
	reg = reg_rxq_int.wrd;
	eof = reg_rxq_int.bf.rxq_eof;
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_COAL_INTERRUPT, reg_rxq_int.wrd);

	reg_rxq_misc_int.wrd = ca77xx_rd_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_MISC_INTERRUPT);
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_MISC_INTERRUPT, reg_rxq_misc_int.wrd);
	if (reg_rxq_misc_int.wrd != 0)
		dev_warn(audio->dev, "reg_rxq_misc_int.wrd=0x%x\n", reg_rxq_misc_int.wrd);

	ca77xx_audio_dma_cnt_update(audio, DIR_RX, reg);

	if (eof) {
		if (audio->ssp_ctrl.type == CA77XX_AUDIO_PCM) {
			audio->dma_ctrl.rx_read_ptr = (audio->dma_ctrl.rx_read_ptr + 1) % audio->dma_ctrl.rx_buf_num;
			ca77xx_audio_dma_rx_update(audio, audio->dma_ctrl.rx_read_ptr);
		}

		if (audio->dma_ctrl.dma_rx_notifier)
			audio->dma_ctrl.dma_rx_notifier(audio->dma_ctrl.rx_data);
	}
#endif
#ifdef CONFIG_RTL8686_IPC_DEV
#ifdef G3_TEST
	if (eof){
		aipc_audio_dma_txrx_notify();
	}
#endif
#endif
	return IRQ_HANDLED;
}

static int ca77xx_audio_dma_init(struct ca77xx_audio *audio)
{
	DMA_SEC_DMA_GLB_DMA_SSP_TX_CTRL_t reg_dma_tx_ctrl;
	DMA_SEC_DMA_GLB_DMA_SSP_RX_CTRL_t reg_dma_rx_ctrl;
	int ret;

	/* turn on DMA top switch */
	reg_dma_rx_ctrl.wrd = ca77xx_rd_dma_glb_reg(audio, CA77XX_DMA_GLB_RX_CTRL);
	reg_dma_rx_ctrl.bf.rx_dma_enable = 1;
#ifndef BUG48545
	reg_dma_rx_ctrl.bf.rx_check_own = 1;
	reg_dma_rx_ctrl.bf.rxdes_update_en = 1;
#endif
	ca77xx_wr_dma_glb_reg(audio, CA77XX_DMA_GLB_RX_CTRL, reg_dma_rx_ctrl.wrd);

	reg_dma_tx_ctrl.wrd = ca77xx_rd_dma_glb_reg(audio, CA77XX_DMA_GLB_TX_CTRL);
	reg_dma_tx_ctrl.bf.tx_dma_enable = 1;
#ifndef BUG48545
	reg_dma_tx_ctrl.bf.tx_check_own = 1;
	reg_dma_tx_ctrl.bf.txdes_update_en = 1;
#endif
	ca77xx_wr_dma_glb_reg(audio, CA77XX_DMA_GLB_TX_CTRL, reg_dma_tx_ctrl.wrd);

	/* disable interrupts */
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_RX_INTERRUPT, 0);
	ca77xx_wr_dma_ssp_reg(audio, CA77XX_DMA_SSP_TX_INTERRUPT, 0);

	ret = devm_request_irq(audio->dev, audio->irq_dma_tx, ca77xx_audio_dma_tx_irq_handler,
			       0, "ca77xx-dma-tx", audio);
	if (ret) {
		dev_err(audio->dev, "could not request dma-tx IRQ: %d\n", ret);
		return ret;
	}

	ret = devm_request_irq(audio->dev, audio->irq_dma_rx, ca77xx_audio_dma_rx_irq_handler,
			       0, "ca77xx-dma-rx", audio);
	if (ret) {
		dev_err(audio->dev, "could not request dma-rx IRQ: %d\n", ret);
		return ret;
	}

	return 0;
}

static int ca77xx_audio_dma_probe(struct platform_device *pdev)
{
	struct ca77xx_audio *audio = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct resource *reg_res;

	reg_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dma-glb");
	audio->dma_glb_base = devm_ioremap_resource(dev, reg_res);
	if (IS_ERR(audio->dma_glb_base))
		return PTR_ERR(audio->dma_glb_base);
	dev_info(dev, "dma glb reg - %pr mapped at 0x%pK\n", reg_res,
		 audio->dma_glb_base);

	reg_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dma-ssp");
	audio->dma_ssp_base = devm_ioremap_resource(dev, reg_res);
	if (IS_ERR(audio->dma_ssp_base))
		return PTR_ERR(audio->dma_ssp_base);
	dev_info(dev, "dma ssp reg - %pr mapped at 0x%pK\n", reg_res,
		 audio->dma_ssp_base);

	audio->irq_dma_tx = platform_get_irq_byname(pdev, "dma-tx");
	if (audio->irq_dma_tx < 0)
		return audio->irq_dma_tx;
	dev_info(dev, "dma-tx irq is %d\n", audio->irq_dma_tx);

	audio->irq_dma_rx = platform_get_irq_byname(pdev, "dma-rx");
	if (audio->irq_dma_rx < 0)
		return audio->irq_dma_rx;
	dev_info(dev, "dma-rx irq is %d\n", audio->irq_dma_rx);

	ca77xx_audio_dma_init(audio);

	return 0;
}

static void ca77xx_audio_ssp_cnt_init(struct ca77xx_audio *audio)
{
	memset(&audio->ssp_ctrl.statis, 0, sizeof(audio->ssp_ctrl.statis));
}

static void ca77xx_audio_ssp_cnt_disp(struct ca77xx_audio *audio, int dir)
{
	dev_info(audio->dev, "tfsFull %d\n", audio->ssp_ctrl.statis.tfsFull);
	dev_info(audio->dev, "tfsEmpty %d\n", audio->ssp_ctrl.statis.tfsEmpty);
	dev_info(audio->dev, "tfwWtMk %d\n", audio->ssp_ctrl.statis.tfwWtMk);
	dev_info(audio->dev, "tfsUndRun %d\n", audio->ssp_ctrl.statis.tfsUndRun);
	dev_info(audio->dev, "rfsFull %d\n", audio->ssp_ctrl.statis.rfsFull);
	dev_info(audio->dev, "rfsEmpty %d\n", audio->ssp_ctrl.statis.rfsEmpty);
	dev_info(audio->dev, "rfrWtMk %d\n", audio->ssp_ctrl.statis.rfrWtMk);
	dev_info(audio->dev, "rfsOvrRun %d\n", audio->ssp_ctrl.statis.rfsOvrRun);
	dev_info(audio->dev, "slic_int %d\n", audio->ssp_ctrl.statis.slic_int);
	dev_info(audio->dev, "pcm_slv_disc %d\n", audio->ssp_ctrl.statis.pcm_slv_disc);
}

static void ca77xx_audio_ssp_cnt_update(struct ca77xx_audio *audio, u32 reg)
{
	DMA_SEC_SSP_INTERRUPT_t reg_ssp_int;

	reg_ssp_int.wrd = reg;

	if (reg_ssp_int.bf.tfsFull)
		audio->ssp_ctrl.statis.tfsFull++;
	if (reg_ssp_int.bf.tfsEmpty)
		audio->ssp_ctrl.statis.tfsEmpty++;
	if (reg_ssp_int.bf.tfwWtMk)
		audio->ssp_ctrl.statis.tfwWtMk++;
	if (reg_ssp_int.bf.tfsUndRun)
		audio->ssp_ctrl.statis.tfsUndRun++;
	if (reg_ssp_int.bf.rfsFull)
		audio->ssp_ctrl.statis.rfsFull++;
	if (reg_ssp_int.bf.rfsEmpty)
		audio->ssp_ctrl.statis.rfsEmpty++;
	if (reg_ssp_int.bf.rfrWtMk)
		audio->ssp_ctrl.statis.rfrWtMk++;
	if (reg_ssp_int.bf.rfsOvrRun)
		audio->ssp_ctrl.statis.rfsOvrRun++;
	if (reg_ssp_int.bf.slic_int)
		audio->ssp_ctrl.statis.slic_int++;
	if (reg_ssp_int.bf.pcm_slv_disc)
		audio->ssp_ctrl.statis.pcm_slv_disc++;
}

static irqreturn_t ca77xx_audio_ssp_irq_handler(int irq, void *dev_instance)
{
	struct ca77xx_audio *audio = (struct ca77xx_audio *)dev_instance;
	u32 val = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_INTERRUPT);

	dev_warn(audio->dev, "ssp irq 0x%x\n", val);

	ca77xx_wr_ssp_reg(audio, CA77XX_SSP_INTERRUPT, val);

	ca77xx_audio_ssp_cnt_update(audio, val);

	return IRQ_HANDLED;
}

static int ca77xx_audio_ssp_init(struct ca77xx_audio *audio)
{
	DMA_SEC_SSP_FIFO_CTRL_t reg_fifo_ctrl;
	DMA_SEC_SSP_INT_ENABLE_t reg_int_enable;
	int ret;

	clk_prepare_enable(audio->clk);

	reg_fifo_ctrl.wrd = ca77xx_rd_ssp_reg(audio, CA77XX_SSP_FIFO_CTRL);
	reg_fifo_ctrl.bf.tfrWtMkLvl = 4;
	reg_fifo_ctrl.bf.tfwWtMkLvl = 127;
	reg_fifo_ctrl.bf.rfrWtMkLvl = 127;
	reg_fifo_ctrl.bf.rfwWtMkLvl = 0;
	ca77xx_wr_ssp_reg(audio, CA77XX_SSP_FIFO_CTRL, reg_fifo_ctrl.wrd);

	ret = devm_request_irq(audio->dev, audio->irq_ssp,
			       ca77xx_audio_ssp_irq_handler, 0, "ca77xx-ssp",
			       audio);
	if (ret) {
		dev_err(audio->dev, "could not request ssp irq: %d\n", ret);
		return ret;
	}

	reg_int_enable.wrd = 0;
	//reg_int_enable.bf.tfsFull_int_en = 1;
	reg_int_enable.bf.tfsEmpty_int_en = 1;
	//reg_int_enable.bf.tfwWtMk_int_en = 1;
	reg_int_enable.bf.tfsUndRun_int_en = 1;
	reg_int_enable.bf.rfsFull_int_en = 1;
	//reg_int_enable.bf.rfsEmpty_int_en = 1;
	//reg_int_enable.bf.rfrWtMk_int_en = 1;
	reg_int_enable.bf.rfsOvrRun_int_en = 1;
	//reg_int_enable.bf.spdif_int_en = 1;
	//reg_int_enable.bf.spdifFifoFull_int_en = 1;
	reg_int_enable.bf.slic_int_en = 1;
	reg_int_enable.bf.pcm_slv_disc_en = 1;
	ca77xx_wr_ssp_reg(audio, CA77XX_SSP_INT_ENABLE, reg_int_enable.wrd);

	return 0;
}

static int ca77xx_audio_ssp_probe(struct platform_device *pdev)
{
	struct ca77xx_audio *audio = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct resource *reg_res;
	int err;

	reg_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ssp");
	audio->ssp_base = devm_ioremap_resource(dev, reg_res);
	if (IS_ERR(audio->ssp_base)) {
		err = PTR_ERR(audio->ssp_base);
		dev_err(&pdev->dev, "could not get ssp base, err %d\n", err);
		return err;
	}
	dev_info(dev, "ssp reg - %pr mapped at 0x%pK\n", reg_res,
		 audio->ssp_base);

	audio->irq_ssp = platform_get_irq_byname(pdev, "ssp");
	if (audio->irq_ssp < 0) {
		err = audio->irq_ssp;
		dev_err(&pdev->dev, "could not get ssp irq, err %d\n", err);
		return err;
	}
	dev_info(dev, "ssp irq is %d\n", audio->irq_ssp);

	audio->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(audio->clk)) {
		err = PTR_ERR(audio->clk);
		dev_err(&pdev->dev, "could not get clk, err %d\n", err);
		return err;
	}
	dev_info(dev, "clock rate %ld\n", clk_get_rate(audio->clk));

	if (!strncmp("ext", __clk_get_name(audio->clk), 3))
		audio->ssp_ctrl.ext_clk = 1;
	else
		audio->ssp_ctrl.ext_clk = 0;

	if (of_property_read_bool(pdev->dev.of_node, "clk-enhance"))
		audio->ssp_ctrl.clk_enhance = 1;
	else
		audio->ssp_ctrl.clk_enhance = 0;

#ifdef DEBUG
	if (0) {
		unsigned long sclk_hz, clk_hz, cdiv;
		u32 dividend, sumValue, baud_cnt_upbond;

		clk_hz = 2048000 * 12;
		sclk_hz = 2048000;

		cdiv = DIV_ROUND_UP(clk_hz, sclk_hz);
		cdiv += (cdiv % 2);

		dividend = CLK_UPBOUND;
		sumValue = dividend / (cdiv / 2);
		baud_cnt_upbond = sumValue * (cdiv / 2);

		dev_dbg(dev, "DBG - ISI: s: 0x%x, b: 0x%0x\n", sumValue,
			baud_cnt_upbond);

		clk_hz = 2048000 * 24;
		sclk_hz = 2048000;

		cdiv = DIV_ROUND_UP(clk_hz, sclk_hz);
		cdiv += (cdiv % 2);

		dividend = CLK_UPBOUND;
		sumValue = dividend / (cdiv / 2);
		baud_cnt_upbond = sumValue * (cdiv / 2);

		dev_dbg(dev, "DBG - ZSI: s: 0x%x, b: 0x%0x\n", sumValue,
			baud_cnt_upbond);
		}
#endif

	ca77xx_audio_ssp_init(audio);

	return 0;
}

static int ca77xx_audio_misc_init(struct ca77xx_audio *audio)
{
	spin_lock_init(&audio->lock);

	audio->ssp_ctrl.type = CA77XX_AUDIO_UNDEF;

	return 0;
}

static int ca77xx_audio_probe(struct platform_device *pdev)
{
	struct ca77xx_audio *audio;

	audio = devm_kzalloc(&pdev->dev, sizeof(*audio), GFP_KERNEL);
	if (!audio)
		return -ENOMEM;
	audio->dev = &pdev->dev;

	platform_set_drvdata(pdev, audio);

	if (ca77xx_audio_ssp_probe(pdev))
		goto fail;

	if (ca77xx_audio_dma_probe(pdev))
		goto fail;

	if (ca77xx_audio_misc_init(audio))
		goto fail;

	return 0;

fail:
	return -ENODEV;
}

static int ca77xx_audio_remove(struct platform_device *pdev)
{
	struct ca77xx_audio *audio = platform_get_drvdata(pdev);

	clk_disable_unprepare(audio->clk);

	return 0;
}

static const struct of_device_id ca77xx_audio_match[] = {
	{ .compatible = "cortina,ca77xx-audio", },
	{ },
};
MODULE_DEVICE_TABLE(of, ca77xx_audio_match);

static struct platform_driver ca77xx_audio_driver = {
	.probe = ca77xx_audio_probe,
	.remove = ca77xx_audio_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = of_match_ptr(ca77xx_audio_match),
	},
};
module_platform_driver(ca77xx_audio_driver);

module_param_named(debug, dbg_flag, uint, 0644);
MODULE_PARM_DESC(debug, "audio module debug flag\n");

module_param_named(pattern, pattern, byte, 0644);
MODULE_PARM_DESC(pattern, "tx pattern\n");

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cortina CA77XX AUDIO driver");
