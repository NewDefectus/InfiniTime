//
// Created by user on 2/16/23.
//

#pragma once
#include <cstdint>
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#undef max
#undef min


//class CPCommandBuilder {
//public:
//  template<typename T> void push(T data);
//  uint16_t size();
//  void clear();
//  unsigned char * data();
//private:
//  std::vector<uint8_t> m_data;
//};

int bleCCCDRegister(uint16_t connectionHandle, uint16_t cccdHandle, ble_gatt_attr_fn *callback, void *cb_arg);
int bleCCCDUnregister(uint16_t connectionHandle, uint16_t cccdHandle, ble_gatt_attr_fn *callback, void *cb_arg);

//template <typename T> void CPCommandBuilder::push(T data) {
//  using Converter = union
//  {
//    T src;
//    uint8_t u8[sizeof(T)];
//  };
//
//  uint16_t size = m_data.size();
//  if (size + sizeof(T) < size)
//  {
//    return ;
//  }
//
//  Converter conv{data};
//  for (auto val : conv.u8)
//  {
//    m_data.push_back(val);
//  }
//}