#include <osal_common.h>
#include <os_core.h>
#include "scfg_parse.h"
#include "scfg_show.h"
#include <scfg.h>
#include <linux/of.h>
#include <linux/bsearch.h>
#include <linux/limits.h>
#include <linux/gfp.h>

static int debug_level = 0x00;
module_param(debug_level, int, 0);
MODULE_PARM_DESC(debug_level, "scfg debug level");

static char *scfg_file = "/config/scfg.txt";
module_param(scfg_file, charp, 0);
MODULE_PARM_DESC(scfg_file, "scfg customerized file");

#define SCFG_DEBUG_UPDATE   (0x01)
#define SCFG_DEBUG_READ     (0x02)
#define SCFG_DEBUG_WRITE    (0x04)
#define SCFG_DEBUG_PREPARSE (0x08)

static ca_uint g_scfg_mutex;

#include <scfg_defs.h>
#include "cfg_array.c"

static int compare_by_name(const void *key, const void *elt)
{
	const struct scfg_cfg_item *item = (const struct scfg_cfg_item *)elt;
	const char *name = (const char *)key;

	return strcmp(name, item->scfg_id);
}

/* Because of backward compatiable, name may be either CFG_ID_XXXX or XXXX.*/
static struct scfg_cfg_item *lookup_config_item_by_id(const char *name)
{
	uint32_t index;
	size_t prefix_len;

	if (kstrtou32(name, 10, &index) == 0) {
		if (index < CA_CFG_ID_MAX) {
			return ca_cfg_id + index;
		}
		return NULL;
	}

	prefix_len = strlen(SCFG_CFG_PREFIX);
	if (strncmp(name, SCFG_CFG_PREFIX, prefix_len) == 0) {
		name += prefix_len;
	}

	return bsearch(name, ca_cfg_id, CA_CFG_ID_MAX,
		       sizeof(struct scfg_cfg_item), compare_by_name);
}

static int scfg_update_by_item(struct scfg_cfg_item *cfg_item, const void *data)
{
	if (debug_level & SCFG_DEBUG_PREPARSE) {
		if (memcpy(cfg_item->pscfg_val, data, cfg_item->scfg_len) != 0) {
			printk(KERN_DEBUG "%s is inconsistent\n",
			       cfg_item->scfg_id);
		}
	}

	memcpy(cfg_item->pscfg_val, data, cfg_item->scfg_len);

	if (debug_level & (SCFG_DEBUG_UPDATE)) {
		cfg_item_printf("update", cfg_item);
	}

	return SCFG_OK;
}

int scfg_update(const char *key, const char *data)
{
	struct scfg_cfg_item *cfg_item;
	ca_uint8 buffer[CFG_ID_VALUE_LEN] = { 0 };

	cfg_item = lookup_config_item_by_id(key);
	if (cfg_item == NULL) {
		printk(KERN_DEBUG "No such configure item:%s\n", key);
		return SCFG_ERR;
	}

	if (cfg_item->parse_func(data, buffer) != 0) {
		printk(KERN_WARNING "ca-scfg: value of %s parse failed\n", key);
		return SCFG_PARSE_ERR;
	}

	return scfg_update_by_item(cfg_item, buffer);;
}

ca_uint32 scfg_read(ca_device_id_t device_id,
		    const ca_int8_t * scfg_id, ca_uint32_t len, void *pbuf)
{
	struct scfg_cfg_item *cfg_item;

	if ((NULL == pbuf) || (0 == len) || (NULL == scfg_id)) {
		printk(KERN_ERR "scfg error: scfg_id[%p], buf[%p], len=[%d]\n",
		       scfg_id, pbuf, len);
		return SCFG_PARAM_ERR;
	}

	cfg_item = lookup_config_item_by_id(scfg_id);
	if (cfg_item == NULL) {
		return SCFG_ERR;
	}

	if (cfg_item->pscfg_val == NULL || len < cfg_item->scfg_len) {
		return SCFG_ERR;
	}

	if (debug_level & SCFG_DEBUG_READ) {
		cfg_item_printf("read", cfg_item);
	}
	memcpy(pbuf, cfg_item->pscfg_val, cfg_item->scfg_len);
	return SCFG_OK;
}

EXPORT_SYMBOL(scfg_read);

ca_status_t ca_scfg_read(ca_device_id_t device_id, ca_int8_t * scfg_id,
			 ca_uint32_t len, ca_uint8_t * pbuf)
{
	return scfg_read(device_id, scfg_id, len, pbuf);
}

EXPORT_SYMBOL(ca_scfg_read);

ca_uint32 scfg_write(ca_device_id_t device_id, const ca_int8_t * scfg_id,
		     ca_uint32_t len, void *pbuf)
{
	struct scfg_cfg_item *item;

	if ((NULL == scfg_id) || (0 == len) || (NULL == pbuf)) {
		printk(KERN_ERR "scfg: scfg_id[%p], buf[%p], len=[%d]\n",
		       scfg_id, pbuf, len);
		return SCFG_PARAM_ERR;
	}

	item = lookup_config_item_by_id(scfg_id);
	if (item == NULL) {
		return SCFG_PARAM_ERR;
	}

	scfg_update_by_item(item, pbuf);

	ca_mutex_lock(g_scfg_mutex);
	write_scfg_file(scfg_file, item);
	ca_mutex_unlock(g_scfg_mutex);

	return SCFG_OK;
}

EXPORT_SYMBOL(scfg_write);

ca_status_t ca_scfg_write(ca_device_id_t device_id, ca_int8_t * scfg_id,
			  ca_uint32_t len, ca_uint8_t * pbuf)
{
	return scfg_write(device_id, scfg_id, len, pbuf);
}

EXPORT_SYMBOL(ca_scfg_write);

static int update_database_from_fdt(void)
{
	struct device_node *scfg_node, *options_node;
	struct property *pp;

	scfg_node = of_find_node_by_path("/ca-scfg");
	if (scfg_node == NULL) {
		return 0;
	}

	if (of_device_is_compatible(scfg_node, "cortina,ca-scfg") <= 0) {
		return 0;
	}

	options_node = of_find_node_by_name(scfg_node, "options");

	for_each_property_of_node(options_node, pp) {
		if (strcmp(pp->name, "name") == 0) {
			continue;
		}

		if (pp->length <= 1) {
			continue;
		}

		scfg_update(pp->name, pp->value);
	}

	return 0;
}

ca_status_t ca_device_config_tlv_get(ca_device_id_t device_id,
				     ca_device_config_tlv_t * p_tlv)
{
	struct scfg_cfg_item *cfg_item;

	if (p_tlv == NULL || p_tlv->type >= CA_CFG_ID_MAX) {
		return SCFG_PARAM_ERR;
	}

	cfg_item = ca_cfg_id + p_tlv->type;

	if (cfg_item->pscfg_val == NULL) {
		return SCFG_ERR;
	}

	memcpy(p_tlv->value, cfg_item->pscfg_val, cfg_item->scfg_len);
	p_tlv->len = cfg_item->scfg_len;

	return CA_E_OK;
}

EXPORT_SYMBOL(ca_device_config_tlv_get);

/*
 * ca_device_config_tlv_set:
 */
ca_status_t ca_device_config_tlv_set(ca_device_id_t device_id,
				     ca_device_config_tlv_t * p_tlv)
{
	struct scfg_cfg_item *cfg_item;

	if (p_tlv == NULL || p_tlv->type >= CA_CFG_ID_MAX) {
		return SCFG_PARAM_ERR;
	}

	cfg_item = ca_cfg_id + p_tlv->type;
	return scfg_write(device_id, cfg_item->scfg_id, p_tlv->len,
			  p_tlv->value);
}

EXPORT_SYMBOL(ca_device_config_tlv_set);

static int __init scfg_init(void)
{
	ca_uint8_t *ptr = scfg_values_mem;
	struct scfg_cfg_item *first = ca_cfg_id;
	struct scfg_cfg_item *last = ca_cfg_id + CA_CFG_ID_MAX;

	printk(KERN_INFO "SCFG initialize start.\n");

	/* Initialize the mutex for scfg module. */
	if (ca_mutex_init(&g_scfg_mutex, "scfg_mutex", 0) != 0) {
		printk(KERN_INFO "ca-scfg: failed to create scfg_mutex\n");
	}

	for (; first != last; ptr += first->scfg_len, ++first) {
		first->pscfg_val = ptr;
	}

	if (parse_scfg_file(scfg_file) != 0) {
		printk(KERN_INFO "load %s failed\n", scfg_file);
	}

	if (debug_level & SCFG_DEBUG_PREPARSE) {
		debug_level &= ~(SCFG_DEBUG_PREPARSE);
	}

	update_database_from_fdt();

#ifdef scfg_debug
	{
		ca_uint8_t dev_mode = 0x0a;
		ca_mac_addr_t mac_addr = { 0 };
		ca_device_config_tlv_t *dev_cfg_tlv = NULL;
		int i = 0;

		//scfg_write(0,CFG_ID_DEV_MODE, 1, &dev_mode);

		scfg_read(0, CFG_ID_DEV_MODE, 1, &dev_mode);
		printk(KERN_DEBUG "test, read dev_mode %d.\n", dev_mode);
		scfg_read(0, CFG_ID_MAC_ADDRESS, 6, &mac_addr);
		printk(KERN_DEBUG
		       "test, read mac_addr %02x:%02x:%02x:%02x:%02x:%02x.\n",
		       mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
		       mac_addr[4], mac_addr[5]);

		dev_cfg_tlv = ca_malloc(sizeof(ca_device_config_tlv_t));
		dev_cfg_tlv->type = CFG_ID_L3QM_AXI_ATTRIB_CPU256_EPP_VALUES;
		ca_device_config_tlv_get(0, dev_cfg_tlv);

		for (; i < dev_cfg_tlv->len; i++) {
			printk(KERN_DEBUG "test, read [%d] [%d] = [%d].\n",
			       dev_cfg_tlv->type, i, dev_cfg_tlv->value[i]);
		}
	}
#endif
	printk(KERN_INFO "SCFG initialize done.\n");

	return CA_E_OK;
}

static void __exit scfg_exit(void)
{
	(void)ca_mutex_destroy(g_scfg_mutex);
	printk(KERN_INFO "scfg_module exit.\n");

	return;
}

MODULE_LICENSE("Proprietary");
module_init(scfg_init);
module_exit(scfg_exit);
