#ifndef SCFG_SHOW_H_B91DCFB9_d760_40f5_991f_04bbe4050c72
#define SCFG_SHOW_H_B91DCFB9_d760_40f5_991f_04bbe4050c72

char *cfg_item_sprintf(char *str, size_t size,
		       const struct scfg_cfg_item *cfg_item);
void cfg_item_printf(const char *prefix, const struct scfg_cfg_item *cfg_item);

#endif
