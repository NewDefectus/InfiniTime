#include "components/ble/AppleNotificationCenterServiceClient.h"
#include <algorithm>
#include "components/ble/NotificationManager.h"
#include "systemtask/SystemTask.h"
#include <nrf_log.h>
#include <stdio.h>

using namespace Pinetime::Controllers;

//#define _NOTIFDBG
#ifdef _NOTIFDBG
#define NOTIF_LOG(...) do \
{ \
  NotificationManager::Notification _notif{}; \
  _notif.size = MIN(snprintf(_notif.message.data(), NotificationManager::MessageSize, __VA_ARGS__), NotificationManager::MessageSize); \
  _notif.category = NotificationManager::Categories::SimpleAlert; \
  notificationManager.Push(std::move(_notif)); \
} while(0)
#else
#define NOTIF_LOG(...) {}
#endif

constexpr ble_uuid128_t AppleNotificationCenterServiceClient::ancsUuid;
constexpr ble_uuid128_t AppleNotificationCenterServiceClient::ancsSourceCharUuid;
constexpr ble_uuid128_t AppleNotificationCenterServiceClient::ancsControlCharUuid;
constexpr ble_uuid128_t AppleNotificationCenterServiceClient::ancsDataCharUuid;
constexpr ble_uuid16_t  AppleNotificationCenterServiceClient::cccdUuid;

constexpr uint16_t TitleLength = 30;
constexpr uint16_t SubtitleLength = 50;
constexpr uint16_t MessageLength = NotificationManager::MessageSize - TitleLength - SubtitleLength - 2; // -2 because of null byte for
constexpr uint16_t MaxBufferSize = MAX(TitleLength, MAX(SubtitleLength, MessageLength));
// others

namespace {
  int OnDiscoveryEventCallback(uint16_t conn_handle, const struct ble_gatt_error* error, const struct ble_gatt_svc* service, void* arg) {
    auto client = static_cast<AppleNotificationCenterServiceClient*>(arg);
    return client->OnDiscoveryEvent(conn_handle, error, service);
  }

  int OnANCSCharacteristicDiscoveredCallback(uint16_t conn_handle,
                                                          const struct ble_gatt_error* error,
                                                          const struct ble_gatt_chr* chr,
                                                          void* arg) {
    auto client = static_cast<AppleNotificationCenterServiceClient*>(arg);
    return client->OnCharacteristicsDiscoveryEvent(conn_handle, error, chr);
  }
  
  int OnANCSDescriptorDiscoveryEventCallback(uint16_t conn_handle,
                                                          const struct ble_gatt_error* error,
                                                          uint16_t chr_val_handle,
                                                          const struct ble_gatt_dsc* dsc,
                                                          void* arg) {
    auto client = static_cast<AppleNotificationCenterServiceClient*>(arg);
    return client->OnDescriptorDiscoveryEventCallback(conn_handle, error, chr_val_handle, dsc);
  }
  int OnANCSSubscribe(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg) {
    auto client = static_cast<AppleNotificationCenterServiceClient*>(arg);
    return client->OnNewAlertSubcribe(conn_handle, error, attr);
  }
}

AppleNotificationCenterServiceClient::AppleNotificationCenterServiceClient(Pinetime::System::SystemTask& systemTask,
                                                 Pinetime::Controllers::NotificationManager& notificationManager)
  : systemTask {systemTask}, notificationManager {notificationManager}, notificationStack{} {
}

bool AppleNotificationCenterServiceClient::OnDiscoveryEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_svc* service) {
  if (service == nullptr && error->status == BLE_HS_EDONE) {
    if (isDiscovered) {
      NRF_LOG_INFO("ANCS Discovery found, starting characteristics discovery");

      ble_gattc_disc_all_chrs(connectionHandle, ancsStartHandle, ancsEndHandle, OnANCSCharacteristicDiscoveredCallback, this);
    } else {
      NRF_LOG_INFO("ANCS not found");
      onServiceDiscovered(connectionHandle);
    }
    return true;
  }

  if (service != nullptr && ble_uuid_cmp(&ancsUuid.u, &service->uuid.u) == 0) {
    NRF_LOG_INFO("ANCS discovered : 0x%x - 0x%x", service->start_handle, service->end_handle);
    ancsStartHandle = service->start_handle;
    ancsEndHandle = service->end_handle;
    isDiscovered = true;
  }
  return false;
}

int AppleNotificationCenterServiceClient::OnNewAlertSubcribe(uint16_t connectionHandle, const ble_gatt_error* error, ble_gatt_attr* attribute) {
  uint8_t value[2];
  value[0] = 1;
  value[1] = 0;
  if (error->status == 0) {
    NRF_LOG_INFO("ANCS New alert subscribe OK");
    if (attribute->handle == ancsDataCCCDHandle)
    {
//      NOTIF_LOG("ANCS New alert subscribe OK - Data");
      isDataReady = true;
      if (!isSourceReady)
      {
        ble_gattc_write_flat(connectionHandle, ancsSourceCCCDHandle, value, sizeof(value), OnANCSSubscribe, this);
      }
    } else if (attribute->handle == ancsSourceCCCDHandle)
    {
//      NOTIF_LOG("ANCS New alert subscribe OK - Source");
      isSourceReady = true;
    }
    if (isSourceReady && isDataReady)
    {
      isReady = true;
    }
  } else {
    NRF_LOG_INFO("ANCS New alert subscribe ERROR");
//    NOTIF_LOG("ANCS New alert subscribe ERROR, status: %d", error->status);
    if (error->status == BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN) || error->status == BLE_ATT_ERR_INSUFFICIENT_AUTHEN)
    {
      ble_gap_security_initiate(connectionHandle);
    }
  }
  if (isReady)
  {
    onServiceDiscovered(connectionHandle);
  }
  return 0;
}

int AppleNotificationCenterServiceClient::OnCharacteristicsDiscoveryEvent(uint16_t connectionHandle,
                                                             const ble_gatt_error* error,
                                                             const ble_gatt_chr* characteristic) {
  if (error->status != 0 && error->status != BLE_HS_EDONE) {
    NRF_LOG_INFO("ANCS Characteristic discovery ERROR");
    onServiceDiscovered(connectionHandle);
    return 0;
  }

  if (characteristic == nullptr && error->status == BLE_HS_EDONE) {
    NRF_LOG_INFO("ANCS Characteristic discovery complete");
    if (isCharacteristicDiscovered)
    {
        ble_gattc_disc_all_dscs(connectionHandle, ancsSourceHandle, ancsEndHandle,
                              OnANCSDescriptorDiscoveryEventCallback, this);

    } else
      onServiceDiscovered(connectionHandle);
  } else {
    if (characteristic != nullptr && ble_uuid_cmp(&ancsSourceCharUuid.u, &characteristic->uuid.u) == 0) {
      NRF_LOG_INFO("ANCS Characteristic discovered : Notification Source");
//      NOTIF_LOG("ANCS Characteristic discovered : Notification Source");
      ancsSourceHandle = characteristic->val_handle;
      isCharacteristicDiscovered = true;
    } else if (characteristic != nullptr && ble_uuid_cmp(&ancsControlCharUuid.u, &characteristic->uuid.u) == 0) {
      NRF_LOG_INFO("ANCS Characteristic discovered : Control Point");
//      NOTIF_LOG("ANCS Characteristic discovered : Control Point");
      ancsControlHandle = characteristic->val_handle;
    } else if (characteristic != nullptr && ble_uuid_cmp(&ancsDataCharUuid.u, &characteristic->uuid.u) == 0) {
      NRF_LOG_INFO("ANCS Characteristic discovered : Data Source");
//      NOTIF_LOG("ANCS Characteristic discovered : Data Source");
      ancsDataHandle = characteristic->val_handle;
    } else
      NRF_LOG_INFO("ANCS Characteristic discovered : 0x%x", characteristic->val_handle);
  }
  return 0;
}

int AppleNotificationCenterServiceClient::OnDescriptorDiscoveryEventCallback(uint16_t connectionHandle,
                                                                const ble_gatt_error* error,
                                                                uint16_t characteristicValueHandle,
                                                                const ble_gatt_dsc* descriptor) {
  if (error->status == 0) {
    if (ble_uuid_cmp(&cccdUuid.u, &descriptor->uuid.u) == 0) {
      NRF_LOG_INFO("ANCS Descriptor discovered : %d", descriptor->handle);
      if (ancsSourceCCCDHandle == 0 && ancsSourceHandle == characteristicValueHandle) {
        ancsSourceCCCDHandle = descriptor->handle;
        isSourceFound = true;
        isDescriptorFound = true;
      } else if (ancsDataCCCDHandle == 0 && ancsDataHandle == characteristicValueHandle) {
        ancsDataCCCDHandle = descriptor->handle;
        isDataFound = true;
        isDescriptorFound = true;
      }
    }
  } else {
    // Runs on end
    if (!isDescriptorFound) {
      onServiceDiscovered(connectionHandle);
    } else {
      isDescriptorFound = isSourceFound && isDataFound;
      if (isDescriptorFound)
      {
        uint8_t value[2];
        value[0] = 1;
        value[1] = 0;
        ble_gattc_write_flat(connectionHandle, ancsDataCCCDHandle, value, sizeof(value), OnANCSSubscribe, this);
      } else if (!isSourceFound) {
        ble_gattc_disc_all_dscs(connectionHandle, ancsSourceHandle, ancsEndHandle,
                                OnANCSDescriptorDiscoveryEventCallback, this);
      } else if (!isDataFound) {
        ble_gattc_disc_all_dscs(connectionHandle, ancsDataHandle, ancsEndHandle,
                                OnANCSDescriptorDiscoveryEventCallback, this);
      }
    }
  }
  return 0;
}

void AppleNotificationCenterServiceClient::NewDataPacket(size_t * off) {
  *off = sizeof(uint8_t) + // command
         sizeof (uint32_t); // uid, ignore for now
    
  messageOffset = 0;
  titleLength = 0;
  subtitleLength = 0;
  messageLength = 0;
  currentNotif = NotificationManager::Notification{};
}

bool AppleNotificationCenterServiceClient::ParseHeader(ble_gap_event* event, size_t * off) {
  if (off == nullptr) {
    return false;
}
  
  uint8_t fieldId = 0;
  if(os_mbuf_copydata(event->notify_rx.om, static_cast<int>(*off), sizeof(uint8_t), &fieldId) == -1) {
    return false;
  }
  *off += sizeof(uint8_t);

  uint16_t fieldLen = 0;
  if(os_mbuf_copydata(event->notify_rx.om, static_cast<int>(*off), sizeof(uint16_t), &fieldLen) == -1) {
    return false;
  }
  *off += sizeof(uint16_t);
  switch (static_cast<NotificationAttributeID>(fieldId)) {
    case NotificationAttributeID::NotificationAttributeIDTitle:
      currentState = ANCSNotificationState::StartedTitle;
      titleLength = MIN(fieldLen, TitleLength);
      NOTIF_LOG("Title case - %d, %d = MIN(%d, %d)", fieldId, titleLength, fieldLen, TitleLength);
      break;
    case NotificationAttributeID::NotificationAttributeIDSubtitle:
      currentState = ANCSNotificationState::StartedSubtitle;
      subtitleLength = MIN(fieldLen, SubtitleLength);
      NOTIF_LOG("Sub case - %d, %d = MIN(%d, %d)", fieldId, subtitleLength, fieldLen, SubtitleLength);
      break;
    case NotificationAttributeID::NotificationAttributeIDMessage:
      currentState = ANCSNotificationState::StartedMessage;
      messageLength = MIN(fieldLen, MessageLength);
      NOTIF_LOG("Message case - %d, %d = MIN(%d, %d)", fieldId, messageLength, fieldLen, MessageLength);
      break;
    default:
      NOTIF_LOG("Default case - %d, %d", fieldId, fieldLen);
      break;
  }
  return true;
}
bool AppleNotificationCenterServiceClient::HandleFieldData(ble_gap_event* event, size_t * off, uint16_t * length) {
  char data[MaxBufferSize + 1];
  
  if (off == nullptr || length == nullptr)
    return false;
  
  // Try to read max packet size (from current position), but no more than the field's length
  size_t availableToRead = MIN((OS_MBUF_PKTLEN(event->notify_rx.om) - *off), *length);
  
  if (os_mbuf_copydata(event->notify_rx.om, static_cast<int>(*off), static_cast<int>(availableToRead), &data) == -1) {
    return false;
  }
  *off += availableToRead;
  *length -= availableToRead;
  (void)snprintf(currentNotif.message.data() + messageOffset, availableToRead + 1, "%s", data);
  messageOffset += availableToRead;
  
  return true;
}

void AppleNotificationCenterServiceClient::EndDataPacket() {
  currentNotif.size = static_cast<uint8_t>(messageOffset + 1);
  currentNotif.category = NotificationManager::Categories::SimpleAlert;
  
  notificationManager.Push(std::move(currentNotif));
  // If this crashes it means you have too many friends, AAT
  systemTask.PushMessage(System::Messages::OnNewNotification);
  titleLength = 0;
  subtitleLength = 0;
  messageLength = 0;
  messageOffset = 0;
  currentState = ANCSNotificationState::Idle;
}

void AppleNotificationCenterServiceClient::OnNotification(ble_gap_event* event) {
  if (!isReady) {
    return ;
  }
  if (event->notify_rx.attr_handle == ancsSourceHandle) {
    NSPacket packet;
    const auto packetLen = OS_MBUF_PKTLEN(event->notify_rx.om);
    if (packetLen != sizeof(packet)) {
      return ;
    }

    os_mbuf_copydata(event->notify_rx.om, 0, sizeof(packet), &packet);
    
    if (packet.eventId == EventID::EventIDNotificationModified) {
      return;
    }
    if (packet.eventId == EventID::EventIDNotificationRemoved &&
        notificationStack.find(packet.notificationUid) != notificationStack.end()) {
        notificationStack.erase(packet.notificationUid);
        return ;
    }
    if (packet.eventFlags.EventFlagPreExisting)
    {
      // We have already seen this notification, this could be because of reconnect with an iOS device
      return ;
    }

    // Request from iOS more data
    if (currentState != ANCSNotificationState::Idle) {
      EndDataPacket();
    }
    
    CPCommandBuilder builder{};
    builder.push(CommandID::CommandIDGetNotificationAttributes);
    builder.push(packet.notificationUid);
    builder.push(NotificationAttributeID::NotificationAttributeIDTitle);
    builder.push((TitleLength)); // max is 100, 25 for title (inc \0), 75 for content, at least for now
//    builder.push(NotificationAttributeID::NotificationAttributeIDSubtitle);
//    builder.push((SubtitleLength));
    builder.push(NotificationAttributeID::NotificationAttributeIDMessage);
    builder.push((MessageLength));
    
    ble_gattc_write_flat(event->notify_rx.conn_handle, ancsControlHandle, builder.data(), builder.size(), nullptr, nullptr);
    currentState = ANCSNotificationState::RequestedInfo;
    // 1. Fix instagram notifications messing up others (?)
    // 2. Support hebrew (V)
    // 2.5. emoji support?
    // 3. Cleanup code (maybe check uid for failsafe?) (V?)
    // 4. Handle calls
    // 5. Battery
    // 6. Music
    
//    if(packet.categoryId == CategoryID::CategoryIDIncomingCall)
//    {
//      currentNotif.category = NotificationManager::Categories::IncomingCall;
//      strcpy(currentNotif.message.data(), "New Call");
//      currentNotif.size = 9;
//      systemTask.PushMessage(System::Messages::OnNewCall);
//    }
  } else if (event->notify_rx.attr_handle == ancsDataHandle) {
    size_t packetLen = OS_MBUF_PKTLEN(event->notify_rx.om);
    size_t offset = 0;
    while (packetLen > offset) {
      switch (currentState) {
        case ANCSNotificationState::Idle:
          return ;
        case ANCSNotificationState::RequestedInfo:
          NewDataPacket(&offset);
          currentState = ANCSNotificationState::FinishedField;
          break;
        case ANCSNotificationState::StartedTitle:
          if (!HandleFieldData(event, &offset, &titleLength)) {
            return ;
          }
          if (0 != titleLength) {
            NOTIF_LOG("Need more data for title - %d", titleLength);
            break ;
          }
          currentState = ANCSNotificationState::FinishedField;
          messageOffset += 1;
          break ;
        case ANCSNotificationState::FinishedField:
          // TODO(user): edge case when ID and length are split between 2 packets
          if(!ParseHeader(event, &offset)) {
            NOTIF_LOG("Failed to parse header at %d", offset);
            return ;
          }
          break;
        case ANCSNotificationState::PreSubtitle:
          if (subtitleLength == 0) {
            currentState = ANCSNotificationState::FinishedField;
            break ;
          }
          currentState = ANCSNotificationState::StartedSubtitle;
          // FALLTHROUGH
        case ANCSNotificationState::StartedSubtitle:
          if (!HandleFieldData(event, &offset, &subtitleLength)) {
            return ;
          }
          if (0 != subtitleLength) {
            break ;
          }
          currentState = ANCSNotificationState::FinishedField;
          *(currentNotif.message.data() + messageOffset) = '\n';
          messageOffset += 1;
          break ;
        case ANCSNotificationState::StartedMessage:
          if (!HandleFieldData(event, &offset, &messageLength)) {
            return ;
          }
          if (0 != messageLength) {
            NOTIF_LOG("Need more data for message - %d", messageLength);
            break ;
          }
          EndDataPacket();
          currentState = ANCSNotificationState::Idle;
          return ;
      }
    }
  }
}

void AppleNotificationCenterServiceClient::Reset() {
  ancsStartHandle = 0;
  ancsEndHandle = 0;
  ancsSourceHandle = 0;
  ancsControlHandle = 0;
  ancsDataHandle = 0;
  titleLength = 0;
  subtitleLength = 0;
  messageLength = 0;
  ancsSourceCCCDHandle = 0;
  ancsDataCCCDHandle = 0;
  isDiscovered = false;
  isCharacteristicDiscovered = false;
  isSourceFound = false;
  isDescriptorFound = false;
  isDataFound = false;
  isReady = false;
  isSourceReady = false;
  isDataReady = false;
  currentState = ANCSNotificationState::Idle;
  notificationStack.clear();
}

void AppleNotificationCenterServiceClient::Discover(uint16_t connectionHandle, std::function<void(uint16_t)> onServiceDiscovered) {
  NRF_LOG_INFO("[ANCS] Starting discovery");
  NOTIF_LOG("[ANCS] Starting discovery");
  this->onServiceDiscovered = onServiceDiscovered;
  ble_gattc_disc_svc_by_uuid(connectionHandle, &ancsUuid.u, OnDiscoveryEventCallback, this);
}

template <typename T> void CPCommandBuilder::push(T data) {
  using Converter = union
  {
    T src;
    uint8_t u8[sizeof(T)];
  };
  
  uint16_t size = m_data.size();
  if (size + sizeof(T) < size)
  {
    return ;
  }
  
  Converter conv{data};
  for (auto val : conv.u8)
  {
    m_data.push_back(val);
  }
}
uint16_t CPCommandBuilder::size() {
  return static_cast<uint16_t>(m_data.size());
}
unsigned char* CPCommandBuilder::data() {
  return m_data.data();
}
