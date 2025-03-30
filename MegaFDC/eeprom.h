// MegaFDC (c) 2023-2025 J. Bogin, http://boginjr.com
// EEPROM helper functions to load and save drives configuration

// there's already one EEPROM.h of the Arduino framework
#ifndef _MEGAFDC_EEPROM_H_
#define _MEGAFDC_EEPROM_H_

bool eepromIsConfigurationPresent();
bool eepromLoadConfiguration();
void eepromStoreConfiguration();
void eepromClearConfiguration();

#endif