#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  AUDIO_EVT_CLICK = 0,
  AUDIO_EVT_START,
  AUDIO_EVT_STOP,
  AUDIO_EVT_SPEED_UP,
  AUDIO_EVT_SPEED_DOWN,
  AUDIO_EVT_PACE_UP,
  AUDIO_EVT_PACE_DOWN,
  AUDIO_EVT_MODE,
  AUDIO_EVT_ERROR,
} audio_evt_t;

void audio_init();
void audio_set_volume(uint8_t vol_0_100);
void audio_mute(bool mute);
void audio_play(audio_evt_t evt);

#ifdef __cplusplus
}
#endif