//
// Created by user on 2/17/23.
//
#include "BleUtils.h"

int bleCCCDRegister(uint16_t connectionHandle, uint16_t cccdHandle, ble_gatt_attr_fn *callback, void *cb_arg) {
  uint8_t value[2];
  value[0] = 1;
  value[1] = 0;
  return ble_gattc_write_flat(connectionHandle, cccdHandle, value, sizeof(value), callback, cb_arg);
}
int bleCCCDUnregister(uint16_t connectionHandle, uint16_t cccdHandle, ble_gatt_attr_fn *callback, void *cb_arg) {
  uint8_t value[2];
  value[0] = 0;
  value[1] = 0;
  return ble_gattc_write_flat(connectionHandle, cccdHandle, value, sizeof(value), callback, cb_arg);
}


//uint16_t CPCommandBuilder::size() {
//  return static_cast<uint16_t>(m_data.size());
//}
//unsigned char* CPCommandBuilder::data() {
//  return m_data.data();
//}
//void CPCommandBuilder::clear() {
//  m_data.clear();
//}