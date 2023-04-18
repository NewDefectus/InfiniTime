//
// Created by user on 2/15/23.
//

#pragma once

#include <cstdint>
#include <string>
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <functional>
#include <host/ble_gap.h>
#include <host/ble_uuid.h>
#undef max
#undef min
#include "IMusicService.h"
#include "NotificationManager.h"
#include "components/ble/BleClient.h"

class AppleMusicService : public Pinetime::Controllers::IMusicService, public Pinetime::Controllers::BleClient {
public:
  explicit AppleMusicService(Pinetime::System::SystemTask& system, Pinetime::Controllers::NotificationManager& notificationManager);
  
//  int OnCommand(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt);

  // Status
  bool Ready() const;
  
  // BLE Stuff
  void Reset();
  void Discover(uint16_t connectionHandle, std::function<void(uint16_t)> lambda) override;
  // callbacks
  void OnDiscoveryEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_svc* service);
  void OnCharDiscoverEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_chr* chr);
  void OnDescriptorDiscoverEvent(uint16_t connectionHandle, const ble_gatt_error* error, uint16_t characteristicValueHandle, const
                                ble_gatt_dsc* descriptor);
  void OnSubscribe(uint16_t connectionHandle, const ble_gatt_error* error, ble_gatt_attr* attribute);
  void OnRegisterUpdate(uint16_t connectionHandle, const ble_gatt_error* error, ble_gatt_attr* attribute);
  void OnNotification(ble_gap_event* event);
  
  // IMusicService
  void event(MusicEvent event) override;
  std::string getArtist() const override;
  std::string getTrack() const override;
  std::string getAlbum() const override;
  int getProgress() const override;
  int getTrackLength() const override;
  float getPlaybackSpeed() const override;
  bool isPlaying() const override;
  
  static constexpr ble_uuid128_t amsUuid {.u {.type = BLE_UUID_TYPE_128}, // 89D3502B-0F36-433A-8EF4-C502AD55F8DC
                                          .value = {0xDC, 0xF8, 0x55, 0xAD, 0x02, 0xC5, 0xF4, 0x8E,
                                                    0x3A, 0x43, 0x36, 0x0F, 0x2B, 0x50, 0xD3, 0x89}
  };
  
private:
  
  // bytes.fromhex('89D3502B-0F36-433A-8EF4-C502AD55F8DC'.replace('-',''))[::-1].hex().upper()
  // UUIDs
  static constexpr ble_uuid128_t amsRemoteUuid {.u {.type = BLE_UUID_TYPE_128}, // 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2
                                                .value = {0xC2, 0x51, 0xCA, 0xF7, 0x56, 0x0E, 0xDF, 0xB8,
                                                          0x8A, 0x4A, 0xB1, 0x57, 0xD8, 0x81, 0x3C, 0x9B}
  };
  
  static constexpr ble_uuid128_t amsUpdateUuid {.u {.type = BLE_UUID_TYPE_128}, // 2F7CABCE-808D-411F-9A0C-BB92BA96C102
                                                .value = {0x02, 0xC1, 0x96, 0xBA, 0x92, 0xBB, 0x0C, 0x9A,
                                                          0x1F, 0x41, 0x8D, 0x80, 0xCE, 0xAB, 0x7C, 0x2F}
  };
  static constexpr ble_uuid128_t amsAttributeUuid {.u {.type = BLE_UUID_TYPE_128}, // C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7
                                                   .value = {0xD7, 0xD5, 0xBB, 0x70, 0xA8, 0xA3, 0xAB, 0xA6,
                                                             0xD8, 0x46, 0xAB, 0x23, 0x8C, 0xF3, 0xB2, 0xC6}
  };
  
  static constexpr ble_uuid16_t cccdUuid = {.u {.type = BLE_UUID_TYPE_16},
                                            .value = BLE_GATT_DSC_CLT_CFG_UUID16};
  
  // Apple Enums, see
  // https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleMediaService_Reference/Appendix/Appendix.html
  enum class RemoteCommandID : uint8_t {
    RemoteCommandIDPlay = 0,
    RemoteCommandIDPause = 1,
    RemoteCommandIDTogglePlayPause = 2,
    RemoteCommandIDNextTrack = 3,
    RemoteCommandIDPreviousTrack = 4,
    RemoteCommandIDVolumeUp = 5,
    RemoteCommandIDVolumeDown = 6,
    RemoteCommandIDAdvanceRepeatMode = 7,
    RemoteCommandIDAdvanceShuffleMode = 8,
    RemoteCommandIDSkipForward = 9,
    RemoteCommandIDSkipBackward = 10,
    RemoteCommandIDLikeTrack = 11,
    RemoteCommandIDDislikeTrack = 12,
    RemoteCommandIDBookmarkTrack = 13,
  };
  enum class EntityID : uint8_t {
    EntityIDPlayer = 0,
    EntityIDQueue = 1,
    EntityIDTrack = 2,
  };
  enum class EntityUpdateFlags : uint8_t {
    EntityUpdateFlagTruncated = (1 << 0),
  };
  enum class PlayerAttributeID : uint8_t {
    PlayerAttributeIDName = 0, // A string containing the localized name of the app.
    PlayerAttributeIDPlaybackInfo = 1,  // A concatenation of three comma-separated values:
                                        // PlaybackState: a string that represents the integer value of the playback state (see enum below)
                                        // PlaybackRate: a string that represents the floating point value of the playback rate.
                                        // ElapsedTime: a string that represents the floating point value of the elapsed time of the
                                        // current track, in seconds, at the moment the value was sent to the MR.
    PlayerAttributeIDVolume = 2, // A string that represents the floating point value of the volume, ranging from 0 (silent) to 1 (full volume).
    // Using the following formula at any time, the MR can calculate from the Playback Rate and Elapsed Time how much time has elapsed
    // during the playback of the track:
    // CurrentElapsedTime = ElapsedTime + ((TimeNow – TimePlaybackInfoWasReceived) * PlaybackRate)
    // This lets MRs implement and display a UI scrubber without needing to poll the MS for continuous updates.
  };
  enum class PlaybackState : char {
    PlaybackStatePaused = '0',
    PlaybackStatePlaying = '1',
    PlaybackStateRewinding = '2',
    PlaybackStateFastForwarding = '3',
  };
  enum class QueueAttributeID : uint8_t {
    QueueAttributeIDIndex = 0, // A string containing the integer value of the queue index, zero-based.
    QueueAttributeIDCount = 1, // A string containing the integer value of the total number of items in the queue.
    QueueAttributeIDShuffleMode = 2, // A string containing the integer value of the shuffle mode. Table A-6 lists the shuffle mode constants.
    QueueAttributeIDRepeatMode = 3, // A string containing the integer value value of the repeat mode. Table A-7 lists the repeat mode constants.
  };
  enum class ShuffleMode : char {
    ShuffleModeOff = '0',
    ShuffleModeOne = '1',
    ShuffleModeAll = '2',
  };
  enum class RepeatMode : char {
    RepeatModeOff = '0',
    RepeatModeOne = '1',
    RepeatModeAll = '2',
  };
  enum class TrackAttributeID : uint8_t {
    TrackAttributeIDArtist = 0, // A string containing the name of the artist.
    TrackAttributeIDAlbum = 1, // A string containing the name of the album.
    TrackAttributeIDTitle = 2, // A string containing the title of the track.
    TrackAttributeIDDuration = 3, // A string containing the floating point value of the total duration of the track in seconds.
  };
  union AttributeID {
    PlayerAttributeID player;
    QueueAttributeID queue;
    TrackAttributeID track;
  };
  
  using UpdatePacket = struct __attribute__((packed)) {
    EntityID entityId;
    AttributeID attributeId;
    EntityUpdateFlags flags;
    char data[];
  };
  
  enum class RegisterUpdateState {
    Reset,
    RegisterTrack,
    RegisterPlayer,
    Ready,
  };
  
  uint16_t eventHandle {};

  std::string artistName {"Apple Music"};
  std::string albumName {};
  std::string trackName {"Not Playing"};

  bool playing {false};

  float trackLength {0};
//  int trackNumber {};
//  int tracksTotal {};
  TickType_t trackProgressUpdateTime {0};
  int elapsedTime = 0;

//  float playbackSpeed {1.0f};

//  bool repeat {false};
//  bool shuffle {false};
  
  RegisterUpdateState registerState = RegisterUpdateState::Reset;
  

  Pinetime::System::SystemTask& m_system;
  // Debug:
  Pinetime::Controllers::NotificationManager& m_notificationManager;
  
  // BLE
  std::function<void(uint16_t)> onServiceDiscovered;
  uint16_t connHandle = 0;
  uint16_t amsStartHandle = 0;
  uint16_t amsEndHandle = 0;
  uint16_t amsRemoteHandle = 0;
  uint16_t amsUpdateHandle = 0;
  uint16_t amsAttributeHandle = 0;
  uint16_t amsRemoteCCCDHandle = 0;
  uint16_t amsUpdateCCCDHandle = 0;
};

