#pragma once

#include <memory>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ScreenList.h"
#include <components/ctf/CtfController.h>

#ifdef _INCLUDE_CON
  #define NUM_SCREENS (6)
#else
  #define NUM_SCREENS (5)
#endif

namespace Pinetime {
  namespace Controllers {
    class DateTime;
    class Battery;
    class BrightnessController;
    class Ble;
  }

  namespace Drivers {
    class Watchdog;
  }

  namespace Applications {
    class DisplayApp;

    namespace Screens {
      class SystemInfo : public Screen {
      public:
        explicit SystemInfo(DisplayApp* app,
                            Pinetime::Controllers::DateTime& dateTimeController,
                            const Pinetime::Controllers::Battery& batteryController,
                            Pinetime::Controllers::BrightnessController& brightnessController,
                            const Pinetime::Controllers::Ble& bleController,
                            const Pinetime::Drivers::Watchdog& watchdog,
                            Pinetime::Controllers::MotionController& motionController,
                            const Pinetime::Drivers::Cst816S& touchPanel);
        ~SystemInfo() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        DisplayApp* app;
        Pinetime::Controllers::DateTime& dateTimeController;
        const Pinetime::Controllers::Battery& batteryController;
        Pinetime::Controllers::BrightnessController& brightnessController;
        const Pinetime::Controllers::Ble& bleController;
        const Pinetime::Drivers::Watchdog& watchdog;
        Pinetime::Controllers::MotionController& motionController;
        const Pinetime::Drivers::Cst816S& touchPanel;

        ScreenList<NUM_SCREENS> screens;

        static bool sortById(const TaskStatus_t& lhs, const TaskStatus_t& rhs);

        std::unique_ptr<Screen> CreateScreen1();
        std::unique_ptr<Screen> CreateScreen2();
        std::unique_ptr<Screen> CreateScreen3();
        std::unique_ptr<Screen> CreateScreen4();
        std::unique_ptr<Screen> CreateScreen5();
#ifdef _INCLUDE_CON
        std::unique_ptr<Screen> CreateScreen6();
#endif
      };
    }
  }
}
