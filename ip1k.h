#ifndef IP1K_H
#define IP1K_H

int ip1k_base;

int ip1k_model;

#define IP1K_IPMODEL_REG (ip1k_model)

#define IP1K_CONTROL (ip1k_base+0x00)
#define IP1K_CONFIG_DATA (ip1k_base+0x02)

#define IP1K_MEM_DATA (ip1k_base+0x14)
#define IP1K_MEM_ADDR (ip1k_base+0x16)

#endif
