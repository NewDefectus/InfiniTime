/*  Copyright (C) 2020-2021 JF, Adam Pigg, Avamander

    This file is part of InfiniTime.

    InfiniTime is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    InfiniTime is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <cstdint>
#include <string>
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#include <host/ble_uuid.h>
#include "IMusicService.h"
#undef max
#undef min

namespace Pinetime {
  namespace Controllers {
    class NimbleController;

    class MusicService {
    public:
      explicit MusicService(NimbleController& nimble);

      void Init();

      int OnCommand(struct ble_gatt_access_ctxt* ctxt);

      void event(MusicEvent event) override;
      std::string getArtist() const override;
      std::string getTrack() const override;
      std::string getAlbum() const override;
      int getProgress() const override;
      int getTrackLength() const override;
      float getPlaybackSpeed() const override;
      bool isPlaying() const override;
      constexpr bool hasExtendedSupport() const override;
      bool isFeatureSupported(MusicEvent feature) const override;

    private:
      struct ble_gatt_chr_def characteristicDefinition[14];
      struct ble_gatt_svc_def serviceDefinition[2];

      uint16_t eventHandle {};

      std::string artistName {"Waiting for"};
      std::string albumName {};
      std::string trackName {"track information.."};

      bool playing {false};

      int trackProgress {0};
      int trackLength {0};
      int trackNumber {};
      int tracksTotal {};
      TickType_t trackProgressUpdateTime {0};

      float playbackSpeed {1.0f};

      bool repeat {false};
      bool shuffle {false};

      NimbleController& nimble;
    };
  }
}
