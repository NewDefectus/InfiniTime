//
// Created by user on 2/15/23.
//
#pragma once

#include <cstdint>
#include <string>
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#include <host/ble_uuid.h>
#undef max
#undef min


namespace Pinetime {
  namespace System {
    class SystemTask;
  }
  namespace Controllers {
    class IMusicService {
    public:
      enum MusicEvent : char {
        EVENT_MUSIC_OPEN = 0xe0,
        EVENT_MUSIC_PLAY = 0x00,
        EVENT_MUSIC_PAUSE = 0x01,
        EVENT_MUSIC_NEXT = 0x03,
        EVENT_MUSIC_PREV = 0x04,
        EVENT_MUSIC_VOLUP = 0x05,
        EVENT_MUSIC_VOLDOWN = 0x06,
        
      };
      
      virtual void event(MusicEvent event);
      virtual std::string getArtist() const;
      virtual std::string getTrack() const;
      virtual std::string getAlbum() const;
      virtual int getProgress() const;
      virtual int getTrackLength() const;
      virtual float getPlaybackSpeed() const;
      virtual bool isPlaying() const;

      enum MusicStatus { NotPlaying = 0x00, Playing = 0x01 };
    };
  }
}