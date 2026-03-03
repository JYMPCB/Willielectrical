#include "lvgl.h"
#include "ui_events.h"
#include "audio_mgr.h"

#define UI_NOOP_EVENT(fn) \
    void fn(lv_event_t * e) { (void)e; audio_play(AUDIO_EVT_CLICK); }

UI_NOOP_EVENT(reinit_ent)
UI_NOOP_EVENT(btnLibreOn)
UI_NOOP_EVENT(btnProgramaOn)
UI_NOOP_EVENT(service_postergar)
UI_NOOP_EVENT(service_aceptar)
UI_NOOP_EVENT(config_loaded)
UI_NOOP_EVENT(change_to_home)
UI_NOOP_EVENT(wifi_btn_toggle)
UI_NOOP_EVENT(event_btn_start_ota)
UI_NOOP_EVENT(event_btn_check_ota)
UI_NOOP_EVENT(btnSetPointSeriesMinus)
UI_NOOP_EVENT(btnSetPointSeriesPlus)
UI_NOOP_EVENT(btnSetPointMacroPause)
UI_NOOP_EVENT(btnSetPointPasadasMinus)
UI_NOOP_EVENT(btnSetPointPasadasPlus)
UI_NOOP_EVENT(btnSetPointMicroPauseMin)
UI_NOOP_EVENT(btnSetPointMicroPauseSeg)
UI_NOOP_EVENT(btnTimeModeClicked)
UI_NOOP_EVENT(btnDistanceModeClicked)
UI_NOOP_EVENT(btnSetPointDistancePasada)
UI_NOOP_EVENT(btnSetPointTimePasadaMin)
UI_NOOP_EVENT(btnSetPointTimePasadaSeg)
UI_NOOP_EVENT(btnProgramOk)
