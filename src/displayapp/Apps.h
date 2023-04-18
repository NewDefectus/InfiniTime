#pragma once
#include "screens/InfiniPaint.h"

namespace Pinetime {
  namespace Applications {
    enum class Apps {
      None,
      Launcher,
      Clock,
      SysInfo,
      FirmwareUpdate,
      FirmwareValidation,
      NotificationsPreview,
      Notifications,
      Timer,
      Alarm,
      FlashLight,
      BatteryInfo,
      Music,
#ifdef _INCLUDE_EXTRAS
      Paint,
      Paddle,
      Twos,
      Navigation,
#endif
      HeartRate,
      StopWatch,
      Metronome,
      Motion,
      Steps,
      PassKey,
      QuickSettings,
      Settings,
      SettingWatchFace,
      SettingTimeFormat,
      SettingDisplay,
      SettingWakeUp,
      SettingSteps,
      SettingSetDateTime,
      SettingChimes,
      SettingShakeThreshold,
      SettingBluetooth,
      Error
    };
  }
}
