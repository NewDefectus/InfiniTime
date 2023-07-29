#pragma once

#include <array>
#include <memory>

#include "displayapp/screens/Screen.h"
#include "displayapp/screens/InfiniPaint.h"
#include "displayapp/screens/ScreenList.h"
#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"
#include "components/ctf/CtfController.h"
#include "components/battery/BatteryController.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/Tile.h"

#ifdef _INCLUDE_CON
#define SCREEN_COUNT (2)
#else
#define SCREEN_COUNT (2)
#endif

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class ApplicationList : public Screen {
      public:
        explicit ApplicationList(DisplayApp* app,
                                 Pinetime::Controllers::Settings& settingsController,
                                 const Pinetime::Controllers::Battery& batteryController,
                                 const Pinetime::Controllers::Ble& bleController,
                                 Controllers::DateTime& dateTimeController);
        ~ApplicationList() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        DisplayApp* app;
        auto CreateScreenList() const;
        std::unique_ptr<Screen> CreateScreen(unsigned int screenNum) const;

        Controllers::Settings& settingsController;
        const Pinetime::Controllers::Battery& batteryController;
        const Pinetime::Controllers::Ble& bleController;
        Controllers::DateTime& dateTimeController;

        static constexpr int appsPerScreen = 6;

        // Increment this when more space is needed
        static constexpr int nScreens = SCREEN_COUNT;

        static constexpr std::array<Tile::Applications, appsPerScreen * nScreens> applications {{
#ifdef _INCLUDE_CON
          {Symbols::calendar, Apps::Sched},
#endif
          {Symbols::stopWatch, Apps::StopWatch},
          {Symbols::clock, Apps::Alarm},
          {Symbols::hourGlass, Apps::Timer},
          {Symbols::shoe, Apps::Steps},
          {Symbols::heartBeat, Apps::HeartRate},
#ifdef _INCLUDE_CON
          {Symbols::crypt, Apps::Crypt},
#endif
//          {Symbols::music, Apps::Music},
#ifdef _INCLUDE_EXTRAS
          {Symbols::paintbrush, Apps::Paint},
          {Symbols::paddle, Apps::Paddle},
          {"2", Apps::Twos},
#ifdef _INCLUDE_EXTRAS_NAV
          {Symbols::map, Apps::Navigation},
#endif
#endif

          {Symbols::drum, Apps::Metronome},
          {Symbols::flashlight_small, Apps::FlashLight},
#ifndef _INCLUDE_CON
          {Symbols::none, Apps::None},
          {Symbols::none, Apps::None},
#endif
          
          // {"M", Apps::Motion},
        }};
        ScreenList<nScreens> screens;
      };
    }
  }
}
