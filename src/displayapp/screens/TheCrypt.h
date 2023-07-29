#pragma once

#include <ctime>
#include <string>

#include "displayapp/screens/Screen.h"
#include "displayapp/screens/TheCryptImage.h"
#include <lvgl/lvgl.h>
#include "components/datetime/DateTimeController.h"
#include "components/ctf/CtfController.h"
#include "utility/DirtyValue.h"

#ifdef _INCLUDE_CON

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class TheCrypt : public Screen {
      public:
        explicit TheCrypt( Controllers::DateTime& dateTimeController);
        ~TheCrypt() override;

      private:
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> currentDateTime {};
        int timePointToTimestamp(std::chrono::system_clock::time_point& tp );
        int checkDate(Controllers::DateTime& dateTimeController);
      };
    }
  }
}
#endif