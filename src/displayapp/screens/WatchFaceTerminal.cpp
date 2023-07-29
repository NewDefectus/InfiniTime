#include <lvgl/lvgl.h>
#include <nrf_log.h>
#include "displayapp/screens/WatchFaceTerminal.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "components/ctf/CtfController.h"
#include "WatchFaceTerminalLogo.h"

using namespace Pinetime::Applications::Screens;

static size_t logo_logo_dimension = 32;
static lv_img_dsc_t logo_logo;

WatchFaceTerminal::WatchFaceTerminal(Controllers::DateTime& dateTimeController,
                                     const Controllers::Battery& batteryController,
                                     const Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificationManager,
                                     Controllers::Settings& settingsController,
                                     Controllers::HeartRateController& heartRateController,
                                     Controllers::MotionController& motionController)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {

  batteryValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(batteryValue, true);
  lv_obj_align(batteryValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -20);

  connectState = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(connectState, true);
  lv_obj_align(connectState, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 40);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_LEFT_MID, 0, -100);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_date, true);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -40);

  label_prompt_1 = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_prompt_1, true);
  lv_obj_align(label_prompt_1, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -80);

  label_ctf = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_ctf, true);
  lv_obj_align(label_ctf, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 60);

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_time, true);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -60);

  logo_logo.header.always_zero = 0;
  logo_logo.header.w = logo_logo_dimension;
  logo_logo.header.h = logo_logo_dimension;
  logo_logo.data_size = logo_logo.header.w * logo_logo.header.h * LV_COLOR_SIZE / 8;
  logo_logo.header.cf = LV_IMG_CF_TRUE_COLOR;
  logo_logo.data = logo_map;

  lv_obj_t *aram_icon = lv_img_create(lv_scr_act(), NULL);
  lv_img_set_src(aram_icon, &logo_logo);
  lv_obj_align(aram_icon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_MID, 0, -9);

  backgroundLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_click(backgroundLabel, true);
  lv_label_set_long_mode(backgroundLabel, LV_LABEL_LONG_CROP);
  lv_obj_set_size(backgroundLabel, 240, 240);
  lv_obj_set_pos(backgroundLabel, 0, 0);
  lv_label_set_text_static(backgroundLabel, "");

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(heartbeatValue, true);
  lv_obj_align(heartbeatValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 20);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(stepValue, true);
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 0);
  

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceTerminal::~WatchFaceTerminal() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

static uint32_t to_timestamp(uint32_t hour, uint32_t minute, uint32_t second)
  {
    return (hour * 60 * 60) + (minute * 60) + second;
  }

void WatchFaceTerminal::Refresh() {
  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    lv_label_set_text_fmt(batteryValue, "[BATT] #00FF00 %d%%", batteryPercentRemaining.Get());
    if (batteryController.IsPowerPresent()) {
      lv_label_ins_text(batteryValue, LV_LABEL_POS_LAST, " CHG");
    }
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if(!bleRadioEnabled.Get()) {
      lv_label_set_text_static(connectState, "[BLE ] #004000 DEAD#");
    } else {
      if (bleState.Get()) {
        lv_label_set_text_static(connectState, "[BLE ]#00FF00  ON#");
      } else {
        lv_label_set_text_static(connectState, "[BLE ]#001000  OFF#");
      }
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    if (notificationState.Get()) {
      lv_label_set_text_static(notificationIcon, "You have mail.");
    } else {
      lv_label_set_text_static(notificationIcon, "");
    }
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    auto hour = dateTimeController.Hours();
    auto minute = dateTimeController.Minutes();
    auto second = dateTimeController.Seconds();
    auto year = dateTimeController.Year();
    auto month = dateTimeController.Month();
    auto dayOfWeek = dateTimeController.DayOfWeek();
    auto day = dateTimeController.Day();

    if (displayedHour != hour || displayedMinute != minute || displayedSecond != second) {
      displayedHour = hour;
      displayedMinute = minute;
      displayedSecond = second;

      bool is_leet = (to_timestamp(13, 37, 16) <= to_timestamp(hour, minute, second)) &&
                     (to_timestamp(hour, minute, second) <= to_timestamp(14, 0, 0));
      bool is_midnight = (to_timestamp(0, 0, 16) <= to_timestamp(hour, minute, second)) &&
                     (to_timestamp(hour, minute, second) <= to_timestamp(4, 0, 0));

      if (is_leet)
      {
          lv_label_set_text_static(label_prompt_1, "#00FFFF 1337##00FF00 @badge:~ $ now");
      }
      else if (is_midnight)
      {
        lv_label_set_text_static(label_prompt_1, "#FF0000 root##00FF00 @badge:~ ! now");
      }
      else
      {
          lv_label_set_text_static(label_prompt_1, "#00FF00 user@badge:~ $ now");
      }

      if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
        char ampmChar[3] = "AM";
        if (hour == 0) {
          hour = 12;
        } else if (hour == 12) {
          ampmChar[0] = 'P';
        } else if (hour > 12) {
          hour = hour - 12;
          ampmChar[0] = 'P';
        }
        auto format = "[TIME] #00ff00 %02d:%02d:%02d %s#";
        if (is_leet)
        {
          format = "[T1M3] #00ffff %02X:%02X:%02X %s#";
        }
        else if (is_midnight)
        {
          format = "[T1M3] #ff0000 %02X:%02X:%02X %s#";
        }
        
        lv_label_set_text_fmt(label_time, format, hour, minute, second, ampmChar);
      }
      else
      {
        auto format = "[TIME] #00ff00 %02d:%02d:%02d#";
        if (is_leet)
        {
          format = "[T1M3] #00ffff %02X:%02X:%02X#";
        }
        else if (is_midnight)
        {
          format = "[T1M3] #ff0000 %02X:%02X:%02X#";
        }
        lv_label_set_text_fmt(label_time, format, hour, minute, second);
      }
    }

    if ((year != currentYear) || (month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {
      lv_label_set_text_fmt(label_date, "[DATE] #007f00 %02d_%02d_%04d#", char(day), char(month), short(year));

      currentYear = year;
      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "[CPU ] #ee3311 %d bpm#", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "[CPU ] #ee3311 ---#");
    }
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "[STEP]#ee3377 %lu steps#", stepCount.Get());
  }

  Pinetime::Controllers::Ctf* ctfController = Pinetime::Controllers::Ctf::getInstance();

  std::string ctf_solved;
  ctfController->getSolved(ctf_solved);

  lv_label_set_text_fmt(label_ctf, "[LVL ] #00FF00 %s#", ctf_solved.c_str());
}
