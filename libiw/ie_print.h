#ifndef IE_PRINTER_H
#define IE_PRINTER_H

#include "ie.h"
#include "ie_he.h"

#ifdef __cplusplus
extern "C" {
#endif

void ie_print_rm_enabled_capabilities(const struct IE_RM_Enabled_Capabilities* sie);
void ie_print_mobility_domain(const struct IE_Mobility_Domain* sie);
void ie_print_he_capabilities(const struct IE_HE_Capabilities* sie);
void ie_print_he_capabilities_mac(const struct IE_HE_MAC* mac);
void ie_print_he_capabilities_phy(const struct IE_HE_PHY* phy);
void ie_print_he_operation(const struct IE_HE_Operation* sie);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

