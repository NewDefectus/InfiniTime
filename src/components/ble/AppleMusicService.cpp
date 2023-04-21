//
// Created by user on 2/15/23.
//

#include <nrf_log.h>

#include "AppleMusicService.h"
#include "BleUtils.h"
#include "systemtask/SystemTask.h"


//#define _NOTIFDBG
//#define _AGGRESSIVEDBG
#ifdef _NOTIFDBG
  #define NOTIF_LOG(...) do \
{ \
  Pinetime::Controllers::NotificationManager::Notification _notif{}; \
  _notif.size = MIN(snprintf(_notif.message.data(), Pinetime::Controllers::NotificationManager::MessageSize, __VA_ARGS__), Pinetime::Controllers::NotificationManager::MessageSize); \
  _notif.category = Pinetime::Controllers::NotificationManager::Categories::SimpleAlert; \
  m_notificationManager.Push(std::move(_notif)); \
} while(0)
#else
  #define NOTIF_LOG(...) {  }
#endif
#ifdef _AGGRESSIVEDBG
#undef NRF_LOG_INFO
  #define NRF_LOG_INFO(...) do \
{ \
  Pinetime::Controllers::NotificationManager::Notification _notif{}; \
  _notif.size = MIN(snprintf(_notif.message.data(), Pinetime::Controllers::NotificationManager::MessageSize, __VA_ARGS__), Pinetime::Controllers::NotificationManager::MessageSize); \
  _notif.category = Pinetime::Controllers::NotificationManager::Categories::SimpleAlert; \
  m_notificationManager.Push(std::move(_notif)); \
} while(0)
#endif

constexpr ble_uuid128_t AppleMusicService::amsUuid;
constexpr ble_uuid128_t AppleMusicService::amsRemoteUuid;
constexpr ble_uuid128_t AppleMusicService::amsUpdateUuid;
constexpr ble_uuid128_t AppleMusicService::amsAttributeUuid;
constexpr ble_uuid16_t  AppleMusicService::cccdUuid;

namespace {
  int OnDiscoveryEventCallback(uint16_t conn_handle, const struct ble_gatt_error* error, const struct ble_gatt_svc* service, void* arg) {
    auto *client = static_cast<AppleMusicService*>(arg);
    client->OnDiscoveryEvent(conn_handle, error, service);
    return 0;
  }
  int OnAMSCharDiscoverCB(uint16_t conn_handle,
                        const struct ble_gatt_error* error,
                        const struct ble_gatt_chr* chr,
                        void* arg) {
    auto *client = static_cast<AppleMusicService*>(arg);
    client->OnCharDiscoverEvent(conn_handle, error, chr);
    return 0;
  }
  int OnAMSDescriptorDiscovery(uint16_t conn_handle,
                                             const struct ble_gatt_error* error,
                                             uint16_t chr_val_handle,
                                             const struct ble_gatt_dsc* dsc,
                                             void* arg) {
    auto client = static_cast<AppleMusicService*>(arg);
    client->OnDescriptorDiscoverEvent(conn_handle, error, chr_val_handle, dsc);
    return 0;
  }
  int OnAMSSubscribe(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg) {
    auto client = static_cast<AppleMusicService*>(arg);
    client->OnSubscribe(conn_handle, error, attr);
    return 0;
    
  }
  int OnAMSRegisterUpdate(uint16_t conn_handle, const struct ble_gatt_error* error, struct ble_gatt_attr* attr, void* arg) {
    auto client = static_cast<AppleMusicService*>(arg);
    client->OnRegisterUpdate(conn_handle, error, attr);
    return 0;
  }
}


AppleMusicService::AppleMusicService(Pinetime::System::SystemTask& system, Pinetime::Controllers::NotificationManager& notificationManager)
  :  m_system {system}, m_notificationManager {notificationManager} {
}

void AppleMusicService::Reset() {
  connHandle = 0;
  amsStartHandle = 0;
  amsEndHandle = 0;
  amsRemoteHandle = 0;
  amsUpdateHandle = 0;
  amsAttributeHandle = 0;
  amsRemoteCCCDHandle = 0;
  amsUpdateCCCDHandle = 0;
//  supportedFeatures = 0;
  registerState = RegisterUpdateState::Reset;
}

void AppleMusicService::Discover(uint16_t connectionHandle, std::function<void(uint16_t)> lambda) {
  NRF_LOG_INFO("[AMS] Starting discovery");
  this->onServiceDiscovered = lambda;
  ble_gattc_disc_svc_by_uuid(connectionHandle, &amsUuid.u, OnDiscoveryEventCallback, this);
}

// Discovery callbacks
void AppleMusicService::OnDiscoveryEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_svc* service) {
  if (service != nullptr && 0 == ble_uuid_cmp(&service->uuid.u, &amsUuid.u)) {
    amsStartHandle = service->start_handle;
    amsEndHandle = service->end_handle;
  } else if (service == nullptr && error->status == BLE_HS_EDONE) {
    if (amsStartHandle != 0 && amsEndHandle != 0) {
      NRF_LOG_INFO("[AMS] Found Apple Music Service, discovering chars");
      ble_gattc_disc_all_chrs(connectionHandle, amsStartHandle, amsEndHandle, OnAMSCharDiscoverCB, this);
    } else {
      onServiceDiscovered(connectionHandle);
    }
  }
}

void AppleMusicService::OnCharDiscoverEvent(uint16_t connectionHandle, const ble_gatt_error* error, const ble_gatt_chr* chr) {
  if (chr != nullptr) {
    uint16_t handle = chr->val_handle;
    const ble_uuid_t * uuid = &chr->uuid.u;
    
    if (0 == ble_uuid_cmp(uuid, &amsRemoteUuid.u)) {
      amsRemoteHandle = handle;
    } else if (0 == ble_uuid_cmp(uuid, &amsUpdateUuid.u)) {
      amsUpdateHandle = handle;
    } else if (0 == ble_uuid_cmp(uuid, &amsAttributeUuid.u)) {
      amsAttributeHandle = handle;
    }
  } else if (error->status == BLE_HS_EDONE) {
    if (amsRemoteHandle != 0 && amsUpdateHandle != 0 && amsAttributeHandle != 0) {
      NRF_LOG_INFO("[AMS] Found all chars, discovering CCCD...");
      ble_gattc_disc_all_dscs(connectionHandle, amsRemoteHandle, amsEndHandle, OnAMSDescriptorDiscovery, this);
    } else {
      onServiceDiscovered(connectionHandle);
    }
  } else if (error->status != 0) {
    NRF_LOG_INFO("[AMS] Unknown error for chars!");
    onServiceDiscovered(connectionHandle);
  }
}

void AppleMusicService::OnDescriptorDiscoverEvent(uint16_t connectionHandle,
                                                             const ble_gatt_error* error,
                                                             uint16_t characteristicValueHandle,
                                                             const ble_gatt_dsc* descriptor) {
  static bool cccdFoundSomething = false;
  
  if (0 == ble_uuid_cmp(&descriptor->uuid.u, &cccdUuid.u)) {
    if (0 == amsRemoteCCCDHandle && amsRemoteHandle == characteristicValueHandle) {
      NRF_LOG_INFO("[AMS] Found remote CCCD!");
      amsRemoteCCCDHandle = descriptor->handle;
    } else if (0 == amsUpdateCCCDHandle && amsUpdateHandle == characteristicValueHandle) {
      NRF_LOG_INFO("[AMS] Found update CCCD!");
      amsUpdateCCCDHandle = descriptor->handle;
    }
    cccdFoundSomething = true;
  } else if (error->status == BLE_HS_EDONE) {
    if (!cccdFoundSomething) {
      // Couldn't find anything, give up
      onServiceDiscovered(connectionHandle);
      return;
    }
    cccdFoundSomething = false;
    uint16_t handle = 0;
    if (0 == amsRemoteCCCDHandle) {
      NRF_LOG_INFO("[AMS] Missing remote CCCD...");
      handle = amsRemoteHandle;
    } else if (0 == amsUpdateCCCDHandle) {
      NRF_LOG_INFO("[AMS] Missing update CCCD...");
      handle = amsUpdateHandle;
    } else {
      // Found everything
      NRF_LOG_INFO("[AMS] Found all CCCDs!");
      bleCCCDRegister(connectionHandle, amsUpdateCCCDHandle, OnAMSSubscribe, this);
      return ;
    }
    ble_gattc_disc_all_dscs(connectionHandle, handle, amsEndHandle, OnAMSDescriptorDiscovery, this);
  } else if (error->status != 0) {
    NRF_LOG_INFO("[AMS] Unknown error for descriptors!");
    onServiceDiscovered(connectionHandle);
  }
}

void AppleMusicService::OnSubscribe(uint16_t connectionHandle, const ble_gatt_error* error, ble_gatt_attr* attribute) {
  if (0 == error->status && amsUpdateCCCDHandle == attribute->handle) {
    NRF_LOG_INFO("[AMS] Subscribed to update");
    bleCCCDRegister(connectionHandle, amsRemoteCCCDHandle, OnAMSSubscribe, this);
    return ;
  } else if (0 == error->status && amsRemoteCCCDHandle == attribute->handle) {
    NRF_LOG_INFO("[AMS] Subscribed!");
    registerState = RegisterUpdateState::RegisterTrack;
    uint8_t packet[] = {
      static_cast<uint8_t>(EntityID::EntityIDTrack),
      static_cast<uint8_t>(TrackAttributeID::TrackAttributeIDArtist),
      static_cast<uint8_t>(TrackAttributeID::TrackAttributeIDTitle),
      static_cast<uint8_t>(TrackAttributeID::TrackAttributeIDAlbum),
      static_cast<uint8_t>(TrackAttributeID::TrackAttributeIDDuration),
    };
    ble_gattc_write_flat(connectionHandle, amsUpdateHandle, packet, sizeof(packet), OnAMSRegisterUpdate, this);
    return;
  } else if (error->status == BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN) || error->status == BLE_ATT_ERR_INSUFFICIENT_AUTHEN) {
    NRF_LOG_INFO("[AMS] Need pairing");
    ble_gap_security_initiate(connectionHandle);
    return ;
  } else if (0 != error->status) {
    NRF_LOG_INFO("[AMS] OnSubscribe unknown error: %d", error->status);
  }
  onServiceDiscovered(connectionHandle);
}
void AppleMusicService::OnRegisterUpdate(uint16_t connectionHandle, const ble_gatt_error* error, ble_gatt_attr* attribute) {
  if (0 == error->status && amsUpdateHandle == attribute->handle) {
    RegisterUpdateState state = registerState;
    
    if (RegisterUpdateState::RegisterTrack == state) {
      // Register for player
      state = RegisterUpdateState::RegisterPlayer;
      uint8_t packet[] = {
        static_cast<uint8_t>(EntityID::EntityIDPlayer),
        static_cast<uint8_t>(PlayerAttributeID::PlayerAttributeIDPlaybackInfo),
      };
      ble_gattc_write_flat(connectionHandle, amsUpdateHandle, packet, sizeof(packet), OnAMSRegisterUpdate, this);
    } else if (RegisterUpdateState::RegisterPlayer == state) {
      // Done
      state = RegisterUpdateState::Ready;
      connHandle = connectionHandle;
    }
    registerState = state;
  } else if (0 != error->status) {
    NRF_LOG_INFO("[AMS] RegisterUpdate unknown error: %d, state: %d", error->status, registerState);
  }
}

void AppleMusicService::OnNotification(ble_gap_event* event) {
  if (!this->Ready()) {
    return;
  }
  if (amsRemoteHandle == event->notify_rx.attr_handle) {
    int pktlen = MIN(OS_MBUF_PKTLEN(event->notify_rx.om), static_cast<int>(RemoteCommandID::RemoteCommandIDMax));
    RemoteCommandID data[static_cast<int>(RemoteCommandID::RemoteCommandIDMax)];
    
    os_mbuf_copydata(event->notify_rx.om, 0, pktlen, data);
    
    supportedFeatures = 0;
    
    for (int i = 0; i < pktlen; ++i) {
      auto command = static_cast<uint8_t>(data[i]);
      
      supportedFeatures |= 1 << (command + 1);
    }
  }
  if (amsUpdateHandle != event->notify_rx.attr_handle) {
    return;
  }
  
  int pktlen = OS_MBUF_PKTLEN(event->notify_rx.om);
  UpdatePacket packet{};
  int offset = 0;
  
  
  os_mbuf_copydata(event->notify_rx.om, offset, sizeof(packet), &packet);
  offset += sizeof (packet);
  int payloadLen = MIN(pktlen - offset, 511);
  
  char data[512];
  
  os_mbuf_copydata(event->notify_rx.om, offset, payloadLen, data);
  data[payloadLen] = 0;
  NOTIF_LOG("Got data: %s", data);
  
  if (EntityID::EntityIDPlayer == packet.entityId) {
    if (PlayerAttributeID::PlayerAttributeIDPlaybackInfo == packet.attributeId.player) {
      // Parse CSV
      char * start = data;
      char * curr = data;
      
      while (*(++curr) != ',') {}
      *curr = 0;
      bool isPlaying = false;
      NOTIF_LOG("start: %c (%s)", *start, start);
      PlaybackState state = static_cast<PlaybackState>(*start);
      if (PlaybackState::PlaybackStatePlaying == state) {
        isPlaying = true;
      }
      ++curr;
      while (*(++curr) != ',') {}
      start = ++curr;
      while (*(++curr) != '\0') {}
      
      playing = isPlaying;
      trackProgressUpdateTime = xTaskGetTickCount();
      int time;
      time = atoi(start);
      if (PlaybackState::PlaybackStatePlaying == state) {
        time += ((xTaskGetTickCount() - trackProgressUpdateTime) / configTICK_RATE_HZ);
      }
      elapsedTime = time;
    }
  } else if (EntityID::EntityIDTrack == packet.entityId) {
    if (TrackAttributeID::TrackAttributeIDTitle == packet.attributeId.track) {
      trackName = data;
    } else if (TrackAttributeID::TrackAttributeIDAlbum == packet.attributeId.track) {
      albumName = data;
    } else if (TrackAttributeID::TrackAttributeIDArtist == packet.attributeId.track) {
      artistName = data;
    } else if (TrackAttributeID::TrackAttributeIDDuration == packet.attributeId.track) {
      trackLength = atoi(data);
    }
  }
}

bool AppleMusicService::Ready() const {
  return RegisterUpdateState::Ready == registerState;
}

void AppleMusicService::event(AppleMusicService::MusicEvent event) {
  if (!this->Ready()) {
    return;
  }
  if (!isFeatureSupported(event)) {
    return ;
  }
  
  RemoteCommandID command = musicEventToRemoteCommand(event);
  
  uint8_t tosend[1];
  tosend[0] = static_cast<uint8_t>(command);
  
  (void)ble_gattc_write_flat(connHandle, amsRemoteHandle, tosend, sizeof(tosend), nullptr, nullptr);
  NOTIF_LOG("Return code: %d, conn: %d, rem: %d, cmd: %d, size: %d", ret, connHandle, amsRemoteHandle, command, sizeof(tosend));
}

std::string AppleMusicService::getArtist() const { return artistName; }
std::string AppleMusicService::getTrack() const { return trackName; }
std::string AppleMusicService::getAlbum() const { return albumName; }
int AppleMusicService::getProgress() const {
  return playing ? elapsedTime + 1 * ((xTaskGetTickCount() - trackProgressUpdateTime) / configTICK_RATE_HZ) : elapsedTime;
}
int AppleMusicService::getTrackLength() const { return trackLength; }
float AppleMusicService::getPlaybackSpeed() const { return 1; }
bool AppleMusicService::isPlaying() const { return playing; }

constexpr bool AppleMusicService::hasExtendedSupport() const {
  return true;
};

AppleMusicService::RemoteCommandID AppleMusicService::musicEventToRemoteCommand(MusicEvent event) const {
  switch (event) {
    case EVENT_MUSIC_PLAY: return RemoteCommandID::RemoteCommandIDPlay;
    case EVENT_MUSIC_PAUSE: return RemoteCommandID::RemoteCommandIDPause;
    case EVENT_MUSIC_NEXT: return RemoteCommandID::RemoteCommandIDNextTrack;
    case EVENT_MUSIC_PREV: return RemoteCommandID::RemoteCommandIDPreviousTrack;
    case EVENT_MUSIC_VOLUP: return RemoteCommandID::RemoteCommandIDVolumeUp;
    case EVENT_MUSIC_VOLDOWN: return RemoteCommandID::RemoteCommandIDVolumeDown;
    case EVENT_MUSIC_REPEAT: return RemoteCommandID::RemoteCommandIDAdvanceRepeatMode;
    case EVENT_MUSIC_SHUFFLE: return RemoteCommandID::RemoteCommandIDAdvanceShuffleMode;
    case EVENT_MUSIC_SEEK_FW: return RemoteCommandID::RemoteCommandIDSkipForward;
    case EVENT_MUSIC_SEEK_BW: return RemoteCommandID::RemoteCommandIDSkipBackward;
    case EVENT_MUSIC_LIKE: return RemoteCommandID::RemoteCommandIDLikeTrack;
    case EVENT_MUSIC_DISLIKE: return RemoteCommandID::RemoteCommandIDDislikeTrack;
    case EVENT_MUSIC_BOOKMARK: return RemoteCommandID::RemoteCommandIDBookmarkTrack;
    default:
      return RemoteCommandID::RemoteCommandIDInvalid;
  }
}

bool AppleMusicService::isFeatureSupported(MusicEvent feature) const {
  RemoteCommandID command = AppleMusicService::musicEventToRemoteCommand(feature);
  
  return 0 != (supportedFeatures & (1 << (static_cast<uint8_t>(command) + 1)));
};