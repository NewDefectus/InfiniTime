#pragma once

#include "displayapp/screens/Screen.h"
#include "lvgl/src/lv_core/lv_obj.h"
#include <lvgl/lvgl.h>
#include <memory>
#include "components/ctf/CtfController.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class Schedule : public Screen {
      public:
        explicit Schedule();
        ~Schedule() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        void refreshText();

        std::size_t scroll_index;
        ::lv_obj_t* entry_what_label;
        ::lv_obj_t* entry_where_label;
        ::lv_obj_t* entry_when_label;
        ::lv_obj_t* entry_length_label;
        ::lv_obj_t* instructions_label;
      };
    }

    template <>
    struct AppTraits<Apps::Sched> {
      static constexpr Apps app = Apps::Sched;
      static constexpr const char* icon = Screens::Symbols::calendar;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::Sched();
      };
    };
  }
}
