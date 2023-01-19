#pragma once

#include <cstdint>

#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#undef max
#undef min
#include "components/ble/AlertNotificationClient.h"
#include "components/ble/AlertNotificationService.h"
#include "components/ble/BatteryInformationService.h"
#include "components/ble/CurrentTimeClient.h"
#include "components/ble/CurrentTimeService.h"
#include "components/ble/DeviceInformationService.h"
#include "components/ble/DfuService.h"
#include "components/ble/FSService.h"
#include "components/ble/HeartRateService.h"
#include "components/ble/ImmediateAlertService.h"
#include "components/ble/MusicService.h"
#include "components/ble/NavigationService.h"
#include "components/ble/ServiceDiscovery.h"
#include "components/ble/MotionService.h"
#include "components/ble/weather/WeatherService.h"
#include "components/fs/FS.h"
#include "AppleNotificationCenterServiceClient.h"

namespace Pinetime {
  namespace Drivers {
    class SpiNorFlash;
  }

  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class Ble;
    class DateTime;
    class NotificationManager;

    class NimbleController {

    public:
      NimbleController(Pinetime::System::SystemTask& systemTask,
                       Ble& bleController,
                       DateTime& dateTimeController,
                       NotificationManager& notificationManager,
                       Battery& batteryController,
                       Pinetime::Drivers::SpiNorFlash& spiNorFlash,
                       HeartRateController& heartRateController,
                       MotionController& motionController,
                       FS& fs);
      void Init();
      void StartAdvertising();
      int OnGAPEvent(ble_gap_event* event);
      void StartDiscovery();

      Pinetime::Controllers::MusicService& music() {
        return musicService;
      };
      Pinetime::Controllers::NavigationService& navigation() {
        return navService;
      };
      Pinetime::Controllers::AlertNotificationService& alertService() {
        return anService;
      };
      Pinetime::Controllers::WeatherService& weather() {
        return weatherService;
      };

      uint16_t connHandle();
      void NotifyBatteryLevel(uint8_t level);

      void RestartFastAdv() {
        fastAdvCount = 0;
      };

      void EnableRadio();
      void DisableRadio();

    private:
      void PersistBond(struct ble_gap_conn_desc& desc);
      void RestoreBond();

      static constexpr const char* deviceName = "InfiniTime";
      Pinetime::System::SystemTask& systemTask;
      Ble& bleController;
      DateTime& dateTimeController;
      Pinetime::Drivers::SpiNorFlash& spiNorFlash;
      FS& fs;
      DfuService dfuService;

      DeviceInformationService deviceInformationService;
      CurrentTimeClient currentTimeClient;
      AppleNotificationCenterServiceClient ancsClient;
      AlertNotificationService anService;
      AlertNotificationClient alertNotificationClient;
      CurrentTimeService currentTimeService;
      MusicService musicService;
      WeatherService weatherService;
      NavigationService navService;
      BatteryInformationService batteryInformationService;
      ImmediateAlertService immediateAlertService;
      HeartRateService heartRateService;
      MotionService motionService;
      FSService fsService;
      ServiceDiscovery serviceDiscovery;

      uint8_t addrType;
      uint16_t connectionHandle = BLE_HS_CONN_HANDLE_NONE;
      uint8_t fastAdvCount = 0;
      uint8_t bondId[16] = {0};

      ble_uuid128_t dfuServiceUuid =
        {
          .u {.type = BLE_UUID_TYPE_128},
          .value = {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x30, 0x15, 0x00, 0x00}
        };

      ble_uuid128_t ancsServiceUuid = {.u {.type = BLE_UUID_TYPE_128},
             .value = {0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4,
                       0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79}
            }; // ANCS
      ble_uuid16_t ancsServiceUuid16 = {.u {.type = BLE_UUID_TYPE_16},
      .value = 0xF431};
    };

    static NimbleController* nptr;
  }
}
