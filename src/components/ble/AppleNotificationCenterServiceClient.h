#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <mutex>
#include <map>
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#undef max
#undef min
#include "components/ble/BleClient.h"
#include "NotificationManager.h"
#include "ICallService.h"

namespace Pinetime {

  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class NotificationManager;

    class AppleNotificationCenterServiceClient : public Pinetime::Controllers::ICallService, public BleClient {
    public:
      explicit AppleNotificationCenterServiceClient(Pinetime::System::SystemTask& systemTask,
                                       Pinetime::Controllers::NotificationManager& notificationManager);


      
      enum class CategoryID : uint8_t
      {
        CategoryIDOther               = 0,
        CategoryIDIncomingCall        = 1,
        CategoryIDMissedCall          = 2,
        CategoryIDVoicemail           = 3,
        CategoryIDSocial              = 4,
        CategoryIDSchedule            = 5,
        CategoryIDEmail               = 6,
        CategoryIDNews                = 7,
        CategoryIDHealthAndFitness    = 8,
        CategoryIDBusinessAndFinance  = 9,
        CategoryIDLocation            = 10,
        CategoryIDEntertainment       = 11,
      };
      enum class EventID : uint8_t
      {
        EventIDNotificationAdded    = 0,
        EventIDNotificationModified = 1,
        EventIDNotificationRemoved  = 2,
      };
      struct __attribute__((packed)) EventFlags
      {
        unsigned EventFlagSilent          : 1;
        unsigned EventFlagImportant       : 1;
        unsigned EventFlagPreExisting     : 1;
        unsigned EventFlagPositiveAction  : 1;
        unsigned EventFlagNegativeAction  : 1;
        unsigned Reserved                 : 3;
      };
      enum class CommandID : uint8_t
      {
        CommandIDGetNotificationAttributes  = 0,
        CommandIDGetAppAttributes           = 1,
        CommandIDPerformNotificationAction  = 2,
      };
      enum class NotificationAttributeID : uint8_t
      {
        NotificationAttributeIDAppIdentifier        = 0,
        NotificationAttributeIDTitle                = 1, //(Needs to be followed by a 2-bytes max length parameter)
        NotificationAttributeIDSubtitle             = 2, //(Needs to be followed by a 2-bytes max length parameter)
        NotificationAttributeIDMessage              = 3, //(Needs to be followed by a 2-bytes max length parameter)
        NotificationAttributeIDMessageSize          = 4,
        NotificationAttributeIDDate                 = 5,
        NotificationAttributeIDPositiveActionLabel  = 6,
        NotificationAttributeIDNegativeActionLabel  = 7,
      };
      enum class AppAttributeID : uint8_t
      {
        AppAttributeIDDisplayName = 0,
      };
      
      enum class ANCSNotificationState
      {
        Idle,
        RequestedInfo,
        StartedTitle,
        FinishedField,
        PreSubtitle,
        StartedSubtitle,
        StartedMessage,
      };
      
      using NSPacket = struct __attribute__((packed)) {
        EventID eventId;
        EventFlags eventFlags;
        CategoryID categoryId;
        uint8_t categoryCount;
        uint32_t notificationUid;
      };
      
      using NotificationBuilder = struct {
        NSPacket packet;
        NotificationManager::Notification notification;
      };
      using NotificationData = struct {
        NSPacket packet;
        NotificationManager::Notification currentNotif;
        uint16_t titleLength = 0;
        uint16_t subtitleLength = 0;
        uint16_t messageLength = 0;
        size_t messageOffset = 0;
      };
      
      bool OnDiscoveryEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_svc* service);
      int OnNewAlertSubcribe(uint16_t connectionHandle, const ble_gatt_error* error, ble_gatt_attr* attribute);
      int OnCharacteristicsDiscoveryEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_chr* characteristic);
      int OnDescriptorDiscoveryEventCallback(uint16_t connectionHandle, const ble_gatt_error* error, uint16_t characteristicValueHandle,
                                                                                   const ble_gatt_dsc* descriptor);
      void OnNotification(ble_gap_event* event);
      void Reset();
      bool Ready() const;
      bool ShouldReset();
      void Discover(uint16_t connectionHandle, std::function<void(uint16_t)> lambda) override;

      void AcceptIncomingCall() override;
      void RejectIncomingCall() override;
      void MuteIncomingCall() override;
      
      static constexpr ble_uuid128_t ancsUuid {.u {.type = BLE_UUID_TYPE_128},
                                               .value = {0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4,
                                                         0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79}
      };
      
    private:

      void NewDataPacket(size_t * offset);
      bool ParseHeader(ble_gap_event* event, size_t * offset);
      bool HandleFieldData(ble_gap_event* event, size_t * offset, uint16_t * length);
      void EndDataPacket();
      
      static constexpr ble_uuid128_t ancsSourceCharUuid = {.u {.type = BLE_UUID_TYPE_128},
                                                           .value = {0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C,
                                                                     0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F}};
      static constexpr ble_uuid128_t ancsControlCharUuid = {.u {.type = BLE_UUID_TYPE_128},
                                                            .value = {0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98,
                                                                      0xA8, 0x49, 0xE1, 0x45, 0xF3, 0xD8, 0xD1, 0x69}};
      static constexpr ble_uuid128_t ancsDataCharUuid = {.u {.type = BLE_UUID_TYPE_128},
                                                         .value = {0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE,
                                                                   0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22}};
      static constexpr ble_uuid16_t cccdUuid = {.u {.type = BLE_UUID_TYPE_16},
                                                .value = BLE_GATT_DSC_CLT_CFG_UUID16};

      uint16_t ancsStartHandle = 0;
      uint16_t ancsEndHandle = 0;
      uint16_t ancsSourceHandle = 0;
      uint16_t ancsControlHandle = 0;
      uint16_t ancsDataHandle = 0;
      uint16_t ancsSourceCCCDHandle = 0;
      uint16_t ancsDataCCCDHandle = 0;

      bool isDiscovered = false;
      Pinetime::System::SystemTask& systemTask;
      Pinetime::Controllers::NotificationManager& notificationManager;
      std::function<void(uint16_t)> onServiceDiscovered;
      bool isCharacteristicDiscovered = false;
      bool isSourceFound = false;
      bool isDataFound = false;
      bool isDescriptorFound = false;
      bool isSourceReady = false;
      bool isDataReady = false;
      bool isReady = false;
      bool requireReset = false;
      std::map<uint32_t, NotificationBuilder> notificationStack;
      ANCSNotificationState currentState;
      NotificationData currentData;
    };
  }
}
