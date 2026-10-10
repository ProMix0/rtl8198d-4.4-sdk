#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <asm/div64.h>
#include <soc/cortina/ca77xx-atu.h>

#define MAX_MODULE_NUM	16

struct atu_module {
	char		*name;
	void __iomem	*reg_base;
	int		type;
};

struct ca77xx_atu {
	struct device	*dev;

	struct atu_module module[MAX_MODULE_NUM];

	/* keep for dump */
	char	module_name[32];
	int	entry_index;
};

struct atu_param {
	u32 input;
	u32 mask;
	u32 output;
};

union atu_ctrl {
	volatile struct {
		u32 enable   :  8; /* bits 7:0 */
		u32 rsrvd1   : 16;
		u32 idx      :  6; /* bits 29:24 */
		u32 r        :  1; /* bits 30:30 */
		u32 w        :  1; /* bits 31:31 */
	} bf;
	volatile u32 wrd;
};

union axi_cfg {
	volatile struct {
		u32 arprot     :  3; /* bits 2:0 */
		u32 rsrvd1     :  1;
		u32 awprot     :  3; /* bits 6:4 */
		u32 rsrvd2     :  1;
		u32 arqos      :  4; /* bits 11:8 */
		u32 awqos      :  4; /* bits 15:12 */
		u32 arcache    :  4; /* bits 19:16 */
		u32 awcache    :  4; /* bits 23:20 */
		u32 user_en    :  1; /* bits 24:24 */
		u32 prot_en    :  1; /* bits 25:25 */
		u32 cache_en   :  1; /* bits 26:26 */
		u32 qos_en     :  1; /* bits 27:27 */
		u32 rsrvd3     :  4;
	} bf;
	volatile u32 wrd;
};

union ahb_cfg {
	struct {
		u32 hprot      :  4; /* bits 3:0 */
		u32 rsrvd1     : 20;
		u32 user_en    :  1; /* bits 24:24 */
		u32 prot_en    :  1; /* bits 25:25 */
		u32 rsrvd2     :  6;
	} bf;
	volatile u32 wrd;
};

#define ATU_CTRL	0x00
#define ATU_INPUT	0x04
#define ATU_MASK	0x08
#define ATU_OUTPUT	0x0C
#define ACE_CFG		0x10
#define AXI_ARUSER	0x14
#define	AXI_AWUSER	0x18
#define	AXI_UPPER	0x1C

#define AHB_HUSER	0x14
#define	AHB_UPPER	0x1C

#define ATU_CTRL_W	BIT(31)
#define ATU_CTRL_R	BIT(30)

#define AXI_STD_STR	"axi_std"
#define AXI_PLUS_STR	"axi_plus"
#define AHB_PLUS_STR	"ahb_plus"

#define T_AXI_STD	0
#define T_AXI_PLUS	1
#define T_AHB_PLUS	2

#define powerof2(x)	(((x) != 0) && (((x) & ((x) - 1)) == 0))

#define busy_wait(cond, timeout)			\
	({						\
	unsigned long end_jiffies = jiffies + timeout;	\
	bool succeeded = false;				\
	do {						\
		if (cond) {				\
			succeeded = true;		\
			break;				\
		}					\
		cpu_relax();				\
	} while (time_before_eq(end_jiffies, jiffies));	\
	succeeded;					\
	})

struct ca77xx_atu *ca77xx_atu;

static int atu_convert(struct atu_attr *atu, struct atu_param *param)
{
	u64 dividend;

	/* atu.size must be equal to 2 ^ (16 + n), n = 0 ~ 15) */
	if (((atu->size % 0x10000) != 0) || (powerof2(atu->size >> 16) != 1)) {
		dev_err(ca77xx_atu->dev, "ATU: size must be 0x1_0000 * 2 ^ n, n = 0 ~ 15\n");
		return -1;
	}

	/* atu.in must be size alignemnt */
	/* using do_div() to avoid "undefined reference to __aeabi_uldivmod"
	 * when compiling for ARM 32-bit target
	 */
	dividend = atu->in;
	if (do_div(dividend, atu->size) != 0) {
		dev_err(ca77xx_atu->dev, "ATU: input address must be size alignment\n");
		return -1;
	}

	/* atu.out must be 2^16 alignment */
	if ((atu->out % 0x10000) != 0) {
		dev_err(ca77xx_atu->dev, "ATU: output address must be 0x1_0000 alignment\n");
		return -1;
	}

	param->input = atu->in >> 16;
	param->mask = ((atu->size - 1) ^ 0xFFFFFFFF) >> 16;
	param->output = atu->out >> 16;

	dev_dbg(ca77xx_atu->dev, "%s - input 0x%x, mask 0x%x, output 0x%x\n",
		__func__, param->input, param->mask, param->output);

	return 0;
}

static void atu_revert(struct atu_attr *atu, struct atu_param *param)
{
	atu->in = ((u64)param->input) << 16;
	atu->size = ((param->mask << 16) ^ 0xFFFFFFFF) + 1;
	atu->out = ((u64)param->output) << 16;

	dev_dbg(ca77xx_atu->dev, "%s - input 0x%llx, size 0x%x, output 0x%llx\n",
		__func__, atu->in, atu->size, atu->out);
}

static struct atu_module *atu_module_get(const char *name)
{
	int i;

	for (i = 0; i < MAX_MODULE_NUM; i++) {
		if (!ca77xx_atu->module[i].name)
			return NULL;

		if (!strcmp(name, ca77xx_atu->module[i].name))
			return &ca77xx_atu->module[i];
	}

	return NULL;
}

int ca77xx_atu_set(const char *name, int idx, struct atu_attr *atu, int enable)
{
	struct atu_param param;
	union atu_ctrl ctrl;
	struct atu_module *module = atu_module_get(name);
	void __iomem *base;
	int type;

	if (!module)
		return -ENXIO;
	base = module->reg_base;
	type = module->type;

	if (idx >= ATU_ENTRY_NUM) {
		dev_err(ca77xx_atu->dev, "ATU: only support  %d entries\n",
			ATU_ENTRY_NUM);

		return -ENODEV;
	}

	if (atu_convert(atu, &param))
		return -EINVAL;

	writel(param.input, base + ATU_INPUT);
	writel(param.mask, base + ATU_MASK);
	writel(param.output, base + ATU_OUTPUT);

	writel(0, base + ACE_CFG);
	if (type == T_AXI_STD || type == T_AXI_PLUS) {
		writel(0, base + AXI_ARUSER);
		writel(0, base + AXI_AWUSER);

		if (type == T_AXI_PLUS)
			writel(0, base + AXI_UPPER);
	} else if (type == T_AHB_PLUS) {
		writel(0, base + AXI_UPPER);
	}

	ctrl.wrd = readl(base + ATU_CTRL);
	ctrl.bf.enable |= (enable << idx);
	ctrl.bf.idx = idx;
	ctrl.bf.w = 1;
	writel(ctrl.wrd, base + ATU_CTRL);
	if (!busy_wait((readl(base + ATU_CTRL) & ATU_CTRL_W) !=
	    ATU_CTRL_W, 10)) {
		dev_err(ca77xx_atu->dev, "ATU: can't set ATU entry %d\n", idx);

		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_atu_set);

int ca77xx_atu_get(const char *name, int idx, struct atu_attr *atu, int *enable)
{
	struct atu_param param;
	union atu_ctrl ctrl;
	struct atu_module *module = atu_module_get(name);
	void __iomem *base;

	if (!module)
		return -ENXIO;
	base = module->reg_base;

	if (idx >= ATU_ENTRY_NUM) {
		dev_err(ca77xx_atu->dev, "ATU: only support  %d entries\n",
			ATU_ENTRY_NUM);

		return -ENODEV;
	}

	ctrl.wrd = readl(base + ATU_CTRL);
	ctrl.bf.idx = idx;
	ctrl.bf.r = 1;
	writel(ctrl.wrd, base + ATU_CTRL);
	if (!busy_wait((readl(base + ATU_CTRL) & ATU_CTRL_R) !=
	    ATU_CTRL_R, 10)) {
		dev_err(ca77xx_atu->dev, "ATU: can't get ATU entry %d\n", idx);

		return -EIO;
	}
	*enable = ctrl.bf.enable & (BIT(idx));

	param.input = readl(base + ATU_INPUT);
	param.mask = readl(base + ATU_MASK);
	param.output = readl(base + ATU_OUTPUT);

	atu_revert(atu, &param);

	return 0;
}
EXPORT_SYMBOL(ca77xx_atu_get);

int ca77xx_atu_del(const char *name, int idx)
{
	union atu_ctrl ctrl;
	struct atu_module *module = atu_module_get(name);
	void __iomem *base;
	int type;

	if (!module)
		return -ENXIO;
	base = module->reg_base;
	type = module->type;

	if (idx >= ATU_ENTRY_NUM) {
		dev_err(ca77xx_atu->dev, "ATU: only support  %d entries\n",
			ATU_ENTRY_NUM);

		return -ENODEV;
	}

	writel(0, base + ATU_INPUT);
	writel(0, base + ATU_MASK);
	writel(0, base + ATU_OUTPUT);

	writel(0, base + ACE_CFG);
	if (type == T_AXI_STD || type == T_AXI_PLUS) {
		writel(0, base + AXI_ARUSER);
		writel(0, base + AXI_AWUSER);

		if (type == T_AXI_PLUS)
			writel(0, base + AXI_UPPER);
	} else if (type == T_AHB_PLUS) {
		writel(0, base + AXI_UPPER);
	}

	ctrl.wrd = readl(base + ATU_CTRL);
	ctrl.bf.enable &= ~(0x1 << idx);
	ctrl.bf.idx = idx;
	ctrl.bf.w = 1;
	writel(ctrl.wrd, base + ATU_CTRL);
	if (!busy_wait((readl(base + ATU_CTRL) & ATU_CTRL_W) !=
	    ATU_CTRL_W, 10)) {
		dev_err(ca77xx_atu->dev, "ATU: can't set ATU entry %d\n", idx);

		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_atu_del);

int ca77xx_ace_set(const char *name, int idx, struct atu_attr *atu,
		   struct ace_attr *ace, int enable)
{
	struct atu_param param;
	union atu_ctrl ctrl;
	union axi_cfg cfg_axi;
	union ahb_cfg cfg_ahb;
	struct atu_module *module = atu_module_get(name);
	void __iomem *base;
	int type;

	if (!module)
		return -ENXIO;
	base = module->reg_base;
	type = module->type;

	if (idx >= ATU_ENTRY_NUM) {
		dev_err(ca77xx_atu->dev, "ATU: only support  %d entries\n",
			ATU_ENTRY_NUM);

		return -ENODEV;
	}

	if (atu_convert(atu, &param))
		return -EINVAL;

	writel(param.input, base + ATU_INPUT);
	writel(param.mask, base + ATU_MASK);
	writel(param.output, base + ATU_OUTPUT);

	if (type == T_AXI_STD || type == T_AXI_PLUS) {
		cfg_axi.wrd = 0;
		cfg_axi.bf.arprot = ace->arprot;
		cfg_axi.bf.awprot = ace->awprot;
		cfg_axi.bf.prot_en = ace->prot_en;
		cfg_axi.bf.arqos = ace->arqos;
		cfg_axi.bf.awqos = ace->awqos;
		cfg_axi.bf.qos_en = ace->qos_en;
		cfg_axi.bf.arcache = ace->arcache;
		cfg_axi.bf.awcache = ace->awcache;
		cfg_axi.bf.cache_en = ace->cache_en;
		cfg_axi.bf.user_en = ace->user_en;
		writel(cfg_axi.wrd, base + ACE_CFG);
		writel(ace->aruser.wrd, base + AXI_ARUSER);
		writel(ace->awuser.wrd, base + AXI_AWUSER);
		if (type == T_AXI_PLUS)
			writel(0, base + AXI_UPPER);
	} else {
		cfg_ahb.wrd = 0;
		cfg_ahb.bf.hprot = ace->hprot;
		cfg_ahb.bf.prot_en = ace->prot_en;
		cfg_ahb.bf.user_en = ace->user_en;
		writel(cfg_ahb.wrd, base + ACE_CFG);
		writel(ace->huser.wrd, base + AHB_HUSER);
		writel(0, base + AHB_UPPER);
	}

	ctrl.wrd = readl(base + ATU_CTRL);
	ctrl.bf.enable |= (enable << idx);
	ctrl.bf.idx = idx;
	ctrl.bf.w = 1;
	writel(ctrl.wrd, base + ATU_CTRL);
	if (!busy_wait((readl(base + ATU_CTRL) & ATU_CTRL_W) !=
	    ATU_CTRL_W, 10)) {
		dev_err(ca77xx_atu->dev, "ATU: can't set ATU entry %d\n", idx);

		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_ace_set);

int ca77xx_ace_get(const char *name, int idx, struct atu_attr *atu,
		   struct ace_attr *ace, int *enable)
{
	struct atu_param param;
	union atu_ctrl ctrl;
	union axi_cfg cfg_axi;
	union ahb_cfg cfg_ahb;
	struct atu_module *module = atu_module_get(name);
	void __iomem *base;
	int type;

	if (!module)
		return -ENXIO;
	base = module->reg_base;
	type = module->type;

	if (idx >= ATU_ENTRY_NUM) {
		dev_err(ca77xx_atu->dev, "ATU: only support  %d entries\n",
			ATU_ENTRY_NUM);

		return -ENODEV;
	}

	ctrl.wrd = readl(base + ATU_CTRL);
	ctrl.bf.idx = idx;
	ctrl.bf.r = 1;
	writel(ctrl.wrd, base + ATU_CTRL);
	if (!busy_wait((readl(base + ATU_CTRL) & ATU_CTRL_R) !=
	    ATU_CTRL_R, 10)) {
		dev_err(ca77xx_atu->dev, "ATU: can't get ATU entry %d\n", idx);

		return -EIO;
	}
	*enable = ctrl.bf.enable & (BIT(idx));

	param.input = readl(base + ATU_INPUT);
	param.mask = readl(base + ATU_MASK);
	param.output = readl(base + ATU_OUTPUT);

	atu_revert(atu, &param);

	cfg_axi.wrd = readl(base + ACE_CFG);
	if (type == T_AXI_STD || type == T_AXI_PLUS) {
		ace->arprot = cfg_axi.bf.arprot;
		ace->awprot = cfg_axi.bf.awprot;
		ace->prot_en = cfg_axi.bf.prot_en;
		ace->arqos = cfg_axi.bf.arqos;
		ace->awqos = cfg_axi.bf.awqos;
		ace->qos_en = cfg_axi.bf.qos_en;
		ace->arcache = cfg_axi.bf.arcache;
		ace->awcache = cfg_axi.bf.awcache;
		ace->cache_en = cfg_axi.bf.cache_en;
		ace->user_en = cfg_axi.bf.user_en;
		ace->aruser.wrd = readl(base + AXI_ARUSER);
		ace->awuser.wrd = readl(base + AXI_AWUSER);
	} else {
		ace->hprot = cfg_ahb.bf.hprot;
		ace->prot_en = cfg_ahb.bf.prot_en;
		ace->user_en = cfg_ahb.bf.user_en;
		ace->huser.wrd = readl(base + AHB_HUSER);
	}

	return 0;
}
EXPORT_SYMBOL(ca77xx_ace_get);

int ca77xx_ace_del(const char *name, int idx)
{
	return ca77xx_atu_del(name, idx);
}
EXPORT_SYMBOL(ca77xx_ace_del);

static ssize_t ca77xx_atu_module_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct ca77xx_atu *ca77xx_atu = dev_get_drvdata(dev);
	ssize_t ret = 0;

	ret += sprintf(buf, "%s\n", ca77xx_atu->module_name);

	return ret;
}

static ssize_t ca77xx_atu_module_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct ca77xx_atu *ca77xx_atu = dev_get_drvdata(dev);
	struct atu_module *module;
	char name[32];
	int rc, i;

	rc = sscanf(buf, "%s", name);
	if (!rc)
		return count;

	module = atu_module_get(name);
	if (!module) {
		dev_info(dev, "only support...\n");
		for (i = 0; i < MAX_MODULE_NUM; i++) {
			if (!ca77xx_atu->module[i].name)
				break;
			dev_info(dev, "%s\n", ca77xx_atu->module[i].name);
		}
	} else {
		sprintf(ca77xx_atu->module_name, "%s", name);
	}

	return count;
}
static DEVICE_ATTR(module, 0664, ca77xx_atu_module_show,
			ca77xx_atu_module_store);

static ssize_t ca77xx_atu_entry_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ca77xx_atu *ca77xx_atu = dev_get_drvdata(dev);
	ssize_t ret = 0;

	ret += sprintf(buf, "%d\n", ca77xx_atu->entry_index);

	return ret;
}

static ssize_t ca77xx_atu_entry_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct ca77xx_atu *ca77xx_atu = dev_get_drvdata(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	if (val <= ATU_ENTRY_NUM)
		ca77xx_atu->entry_index = val;

	return count;
}
static DEVICE_ATTR(entry, 0664, ca77xx_atu_entry_show,
			ca77xx_atu_entry_store);

static ssize_t ca77xx_atu_dump_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct ca77xx_atu *ca77xx_atu = dev_get_drvdata(dev);
	struct atu_module *module;
	struct atu_attr atu;
	struct ace_attr ace;
	int enable;
	ssize_t ret = 0;

	module = atu_module_get(ca77xx_atu->module_name);
	if (!module)
		return ret;

	if (ca77xx_ace_get(ca77xx_atu->module_name,
			   ca77xx_atu->entry_index, &atu, &ace, &enable))
		return 0;

	ret += sprintf(buf + ret, "module: %s - entry %d\n",
		       ca77xx_atu->module_name, ca77xx_atu->entry_index);
	ret += sprintf(buf + ret, "%s\n", enable ? "enable" : "disable");

	ret += sprintf(buf + ret, "atu - in 0x%llx, out 0x%llx, size 0x%x\n",
		       atu.in, atu.out, atu.size);
	if (module->type == T_AXI_STD || module->type == T_AXI_PLUS) {
		ret += sprintf(buf + ret,
			       "axi - cache_en %d, arcache 0x%x, awcache 0x%x\n",
			       ace.cache_en, ace.arcache, ace.awcache);
		ret += sprintf(buf + ret,
			       "axi - qos_en %d, arqos 0x%x, awqos 0x%x\n",
			       ace.qos_en, ace.arqos, ace.awqos);
		ret += sprintf(buf + ret,
			       "axi - prot_en %d, arprot 0x%x, awprot 0x%x\n",
			       ace.prot_en, ace.arprot, ace.awprot);
		ret += sprintf(buf + ret, "axi - user_en %d\n", ace.user_en);
		ret += sprintf(buf + ret,
			       "axi aruser - axuser %d, axdomain %d, axbar %d, axsnoop %d\n",
			       ace.aruser.bf.axuser, ace.aruser.bf.axdomain,
			       ace.aruser.bf.axbar, ace.aruser.bf.axsnoop);
		ret += sprintf(buf + ret,
			       "axi awuser - axuser %d, axdomain %d, axbar %d, axsnoop %d\n",
			       ace.awuser.bf.axuser, ace.awuser.bf.axdomain,
			       ace.awuser.bf.axbar, ace.awuser.bf.axsnoop);
	} else {
		ret += sprintf(buf + ret, "ahb - prot_en %d, hprot 0x%x\n",
			       ace.prot_en, ace.hprot);
		ret += sprintf(buf + ret, "ahb - user_en %d\n", ace.user_en);
		ret += sprintf(buf + ret,
			       "ahb user - user %d, domain %d, bar %d, snoop %d\n",
			       ace.huser.bf.axuser, ace.huser.bf.axdomain,
			       ace.huser.bf.axbar, ace.huser.bf.axsnoop);
	}

	return ret;
}
static DEVICE_ATTR(dump, 0444, ca77xx_atu_dump_show, NULL);

static struct attribute *ca77xx_atu_attributes[] = {
	&dev_attr_module.attr,
	&dev_attr_entry.attr,
	&dev_attr_dump.attr,
	NULL
};

static const struct attribute_group ca77xx_atu_attr_group = {
	.attrs = ca77xx_atu_attributes,
};

static int __init ca77xx_atu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *reg_res;
	const char *type_name;
	int i, ret, final_ret;

	ca77xx_atu = devm_kzalloc(dev, sizeof(*ca77xx_atu), GFP_KERNEL);
	if (!ca77xx_atu)
		return -ENOMEM;
	ca77xx_atu->dev = dev;

	final_ret = -ENODEV;
	for (i = 0; i < MAX_MODULE_NUM; i++) {
		reg_res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!reg_res)
			break;
		ca77xx_atu->module[i].reg_base =
			devm_ioremap_resource(dev, reg_res);
		if (IS_ERR(ca77xx_atu->module[i].reg_base)) {
			ret = PTR_ERR(ca77xx_atu->module[i].reg_base);
			break;
		}

		if (!reg_res->name)
			break;
		ca77xx_atu->module[i].name = reg_res->name;

		ret = of_property_read_string_index(pdev->dev.of_node,
						    "atu-type", i, &type_name);
		if (ret < 0) {
			ca77xx_atu->module[i].name = NULL;
			break;
		}

		if (!strcmp(type_name, AXI_STD_STR)) {
			ca77xx_atu->module[i].type = T_AXI_STD;
		} else if (!strcmp(type_name, AXI_PLUS_STR)) {
			ca77xx_atu->module[i].type = T_AXI_PLUS;
		} else if (!strcmp(type_name, AHB_PLUS_STR)) {
			ca77xx_atu->module[i].type = T_AHB_PLUS;
		} else {
			ca77xx_atu->module[i].name = NULL;
			break;
		}

		dev_info(dev, "%s(%s) resource - %pr mapped at 0x%pK\n",
			 ca77xx_atu->module[i].name, type_name,
			 reg_res, ca77xx_atu->module[i].reg_base);

		if (final_ret && !ret)
			final_ret = 0;
	}
	if (final_ret)
		return final_ret;

	final_ret = sysfs_create_group(&ca77xx_atu->dev->kobj,
				       &ca77xx_atu_attr_group);
	if (final_ret) {
		dev_err(dev, "failed to register sysfs\n");
		return final_ret;
	}

	platform_set_drvdata(pdev, ca77xx_atu);

	return final_ret;
}

static int __exit ca77xx_atu_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &ca77xx_atu_attr_group);

	return 0;
}

static const struct of_device_id ca77xx_atu_of_match[] = {
	{ .compatible = "cortina,ca77xx-atu", },
	{},
};
MODULE_DEVICE_TABLE(of, ca77xx_atu_of_match);

static struct platform_driver ca77xx_atu_driver = {
	.remove		= __exit_p(ca77xx_atu_remove),
	.driver = {
		.name	= "ca77xx-atu",
		.owner	= THIS_MODULE,
		.of_match_table = ca77xx_atu_of_match,
	},
};

module_platform_driver_probe(ca77xx_atu_driver, ca77xx_atu_probe);
MODULE_DESCRIPTION("Cortina-Access CA77XX ATU-ACE driver");
MODULE_LICENSE("GPL v2");

