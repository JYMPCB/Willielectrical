
#include <lvgl.h>
#include "ui_refresh.h"
#include "ui_state.h"
#include "training_interval.h"
#include "app_globals.h"
#include "ui.h"
#include "ota_mgr.h"
#include "esp_log.h"
#include <time.h>

static const char *TAG = "ui_refresh";

static bool first_refresh = true;

// Flag RAM para no abrir el popup 20 veces en el refresh
static bool s_service_evaluated_this_boot = false;  // evaluar vencimiento (1 vez)
static bool s_service_popup_attempted_this_boot = false; // mostrar popup (1 vez)
static bool s_ota_callbacks_bound = false;
static bool s_ota_checked_this_boot = false;

static void service_popup_show() {
  // Mostrar panel (popup)
  lv_obj_clear_flag(ui_pnlServiceOdo, LV_OBJ_FLAG_HIDDEN);
}

static void service_popup_hide() {
  lv_obj_add_flag(ui_pnlServiceOdo, LV_OBJ_FLAG_HIDDEN);
}

// helper: hora válida si > 2023 aprox
static bool net_time_valid()
{
  time_t now = time(nullptr);
  return (now > 1700000000);
}

static void service_popup_update_text()
{
  char buf[128];

  if(net_time_valid()) {
    time_t now = time(nullptr);
    g_service.formatPopupText(buf, sizeof(buf), now);  // <- función real del ServiceMgr
  } else {
    strlcpy(buf, "Mantenimiento requerido.\n(Sin hora valida)", sizeof(buf));
  }

  lv_label_set_text(ui_lblOdoMaquina2, buf);
}

//---------ARO RGB-------------
extern lv_obj_t * ui_rgbRing;

static inline void ring_apply_color(uint8_t r, uint8_t g, uint8_t b){
  if(!ui_rgbRing) return;

  static uint32_t last_rgb = 0xFFFFFFFF;
  uint32_t rgb = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
  if(rgb == last_rgb) return;
  last_rgb = rgb;

  lv_color_t col = lv_color_make(r,g,b);
  lv_obj_set_style_bg_img_recolor(ui_rgbRing, col, LV_PART_MAIN);
  lv_obj_set_style_bg_img_recolor_opa(ui_rgbRing, LV_OPA_COVER, LV_PART_MAIN);
}

static void ui_net_time_update(ui_state_t &s)
{
  if(!wifi_ok) {
    s.showNetTime = false;
    return;
  }

  if(!net_time_valid()) {
    s.showNetTime = false;
    return;
  }

  s.showNetTime = true;

  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);

  strftime(s.timeStr, sizeof(s.timeStr), "%H:%M", &t);
  strftime(s.dateStr, sizeof(s.dateStr), "%d/%m/%Y", &t);
}

// --------- iconos wifi ----------
static void wifi_icons_apply(bool ok)
{
  lv_color_t c = ok ? lv_color_hex(0xFFBF00) : lv_color_hex(0x525552);

  if(ui_imgWifi)  lv_obj_set_style_img_recolor(ui_imgWifi,  c, LV_PART_MAIN | LV_STATE_DEFAULT);
  if(ui_imgWifi2) lv_obj_set_style_img_recolor(ui_imgWifi2, c, LV_PART_MAIN | LV_STATE_DEFAULT);
}

//helper para boton conectar wifi en pantalla configuracion
static void wifi_btn_apply(bool ok)
{
  if(!ui_btnWifi) return;

  if(ok) lv_obj_add_state(ui_btnWifi, LV_STATE_CHECKED);
  else   lv_obj_clear_state(ui_btnWifi, LV_STATE_CHECKED);
}

static bool is_masked_password(const char *text)
{
  if(!text || text[0] == '\0') return false;
  for(const char *cursor = text; *cursor; ++cursor) {
    if(*cursor != '*') return false;
  }
  return true;
}

// ---------------- PROGRAMADO helpers ----------------
static void programado_apply_mode(bool dist)
{
  if(!ui_btnDistanceMode || !ui_btnTimeMode) return;

  if(dist){
    lv_obj_add_state(ui_btnDistanceMode, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_btnTimeMode, LV_STATE_CHECKED);

    if(ui_ContainerTiempoPasadaMin) lv_obj_add_flag(ui_ContainerTiempoPasadaMin, LV_OBJ_FLAG_HIDDEN);
    if(ui_ContainerTiempoPasadaSeg) lv_obj_add_flag(ui_ContainerTiempoPasadaSeg, LV_OBJ_FLAG_HIDDEN);
    if(ui_ContainerDistancePasada)  lv_obj_clear_flag(ui_ContainerDistancePasada, LV_OBJ_FLAG_HIDDEN);

    if(ui_lblDistanceModeOk) lv_obj_clear_flag(ui_lblDistanceModeOk, LV_OBJ_FLAG_HIDDEN);
    if(ui_lblTimeModeOk)     lv_obj_add_flag(ui_lblTimeModeOk, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_state(ui_btnTimeMode, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_btnDistanceMode, LV_STATE_CHECKED);

    if(ui_ContainerTiempoPasadaMin) lv_obj_clear_flag(ui_ContainerTiempoPasadaMin, LV_OBJ_FLAG_HIDDEN);
    if(ui_ContainerTiempoPasadaSeg) lv_obj_clear_flag(ui_ContainerTiempoPasadaSeg, LV_OBJ_FLAG_HIDDEN);
    if(ui_ContainerDistancePasada)  lv_obj_add_flag(ui_ContainerDistancePasada, LV_OBJ_FLAG_HIDDEN);

    if(ui_lblTimeModeOk)     lv_obj_clear_flag(ui_lblTimeModeOk, LV_OBJ_FLAG_HIDDEN);
    if(ui_lblDistanceModeOk) lv_obj_add_flag(ui_lblDistanceModeOk, LV_OBJ_FLAG_HIDDEN);
  }
}

static void programado_refresh_labels()
{
  if(ui_lblSetPointNumSeries)      lv_label_set_text_fmt(ui_lblSetPointNumSeries, "%u", (unsigned)setPointSeries);
  if(ui_lblSetPointNumPasadas)     lv_label_set_text_fmt(ui_lblSetPointNumPasadas, "%u", (unsigned)setPointPasadas);

  if(ui_lblSetPointMacroPause)     lv_label_set_text_fmt(ui_lblSetPointMacroPause, "%u min", (unsigned)setPointMacroPausaMin);
  if(ui_lblSetPointMicropausaMin)  lv_label_set_text_fmt(ui_lblSetPointMicropausaMin, "%u min", (unsigned)setPointMicroPausaMin);
  if(ui_lblSetPointMicropausaSeg)  lv_label_set_text_fmt(ui_lblSetPointMicropausaSeg, "%u s",   (unsigned)setPointMicroPausaSeg);

  if(ui_lblSetPointDistancePasada) lv_label_set_text_fmt(ui_lblSetPointDistancePasada, "%u mt", (unsigned)setPointDistancePasada);
  if(ui_lblSetPointTimePasadaMin)  lv_label_set_text_fmt(ui_lblSetPointTimePasadaMin, "%u min", (unsigned)setPointTimePasadaMin);
  if(ui_lblSetPointTimePasadaSeg)  lv_label_set_text_fmt(ui_lblSetPointTimePasadaSeg, "%u s",   (unsigned)setPointTimePasadaSeg);

  if(ui_lblInfoTraining){
    if(g_interval_is_dist){
      lv_label_set_text_fmt(ui_lblInfoTraining,
        "Series: %u (pausa %u min) | Pasadas: %u (pausa %u:%02u) | Modo: Distancia %u m",
        (unsigned)setPointSeries,
        (unsigned)setPointMacroPausaMin,
        (unsigned)setPointPasadas,
        (unsigned)setPointMicroPausaMin,
        (unsigned)setPointMicroPausaSeg,
        (unsigned)setPointDistancePasada
      );
    } else {
      lv_label_set_text_fmt(ui_lblInfoTraining,
        "Series: %u (pausa %u min) | Pasadas: %u (pausa %u:%02u) | Modo: Tiempo %u:%02u",
        (unsigned)setPointSeries,
        (unsigned)setPointMacroPausaMin,
        (unsigned)setPointPasadas,
        (unsigned)setPointMicroPausaMin,
        (unsigned)setPointMicroPausaSeg,
        (unsigned)setPointTimePasadaMin,
        (unsigned)setPointTimePasadaSeg
      );
    }
  }
}

// TIMER LVGL: acá van TODAS las sentencias LVGL
extern "C" void ui_refresh_cb(lv_timer_t *t)
{
  (void)t;

  ui_state_t s;

  bool config_active = (habConfig == 1) || (ui_CONFIG && lv_scr_act() == ui_CONFIG);
  bool home_active = (ui_HOME && lv_scr_act() == ui_HOME);

  if (config_active && !habConfig) {
    habConfig = true;
    cfg_entered = true;
  } else if (home_active && habConfig) {
    habConfig = false;
  }

  xSemaphoreTake(g_ui_mutex, portMAX_DELAY);
  s = g_ui;
  xSemaphoreGive(g_ui_mutex);

  // --------- CACHE ----------
  static bool last_headerHidden   = false;
  static bool last_settingsHidden = false;

  static bool last_showNetTime = false;
  static char last_timeStr[8]  = "";
  static char last_dateStr[11] = "";

  static char last_speedStr[16] = "";
  static int  last_cursorAngle  = -1;

  static char last_ritmoStr[16] = "";

  static char last_distStr[16] = "";
  static char last_distUnit[4]  = "";

  static char last_calStr[16]   = "";
  static char last_trainTimeStr[16] = "";

  static bool last_pausaVisible = false;
  static bool last_pausaIsSerie = false;
  static char last_pausaTitle[24] = "";
  static char last_pausaStr[8]  = "";

  static char last_intervalSeriesStr[8]  = "";
  static char last_intervalPasadasStr[8] = "";

  static bool last_habConfig = false;
  static char last_cfg_info[64] = "";
  static char last_cfg_ip[24]   = "";
  static char last_cfg_ssid[33] = "";
  static bool btn_checked_inited = false;
  static bool last_btn_checked = false;

  static bool last_wifi_ok = false;

  static char last_odoStr[32] = "";

  // Copias locales
  char info_l[64], ip_l[24], ssid_l[33];
  strlcpy(info_l, cfg_info, sizeof(info_l));
  strlcpy(ip_l,   cfg_ip,   sizeof(ip_l));
  strlcpy(ssid_l, cfg_ssid, sizeof(ssid_l));

  /* --- OTA --- */
  static bool last_ota_active = false;
  static int  last_ota_prog   = -1;
  static char last_ota_status[64] = "";
  static char last_ota_pct[8] = "";

  // 0) Mostrar/ocultar overlay SOLO si cambia el estado
  if (ui_otaOverlay && (g_ota_active != last_ota_active)) {
    if (g_ota_active) lv_obj_clear_flag(ui_otaOverlay, LV_OBJ_FLAG_HIDDEN);
    else              lv_obj_add_flag(ui_otaOverlay, LV_OBJ_FLAG_HIDDEN);
    last_ota_active = g_ota_active;

    // al entrar en OTA, forzá un refresh de cachés OTA
    last_ota_prog = -1;
    last_ota_status[0] = '\0';
    last_ota_pct[0] = '\0';
  }

  // 1) (Opcional) suavizado para que no “salte”
  //    - el real es g_ota_progress
  //    - el que se muestra es disp_prog
  static int disp_prog = 0;
  int target = g_ota_progress;
  if (target < 0) target = 0;
  if (target > 100) target = 100;

  if (!g_ota_active) disp_prog = 0;          // al salir de OTA, reset
  else {
    // sube de a 1-2% por tick para que sea suave
    if (disp_prog < target) disp_prog += 2;
    if (disp_prog > target) disp_prog = target;
  }

  // 2) Progreso (solo si cambia)
  if (g_ota_active && ui_barOta && (disp_prog != last_ota_prog)) {
    lv_bar_set_value(ui_barOta, disp_prog, LV_ANIM_OFF);
    last_ota_prog = disp_prog;
  }

  // 3) Status (solo si cambia) -> en lblOtaNotes
  if (g_ota_active && ui_lblOtaNotes) {
    // g_ota_status debería ser null-terminated
    if (strncmp(last_ota_status, g_ota_status, sizeof(last_ota_status)) != 0) {
      strlcpy(last_ota_status, g_ota_status, sizeof(last_ota_status));
      lv_label_set_text(ui_lblOtaNotes, last_ota_status);
    }
  }

  // 4) Porcentaje (solo si cambia)
  if (g_ota_active && ui_lblOtaPct) {
    char pct[8];
    snprintf(pct, sizeof(pct), "%d%%", disp_prog);

    if (strcmp(last_ota_pct, pct) != 0) {
      strlcpy(last_ota_pct, pct, sizeof(last_ota_pct));
      lv_label_set_text(ui_lblOtaPct, last_ota_pct);
    }
  }

  // Durante OTA, NO refrescar nada más
  if (g_ota_active) return;

  if(first_refresh) {
    first_refresh = false;

    last_cfg_ssid[0] = '\0';
    last_timeStr[0]  = '\0';
    last_dateStr[0]  = '\0';
    last_wifi_ok     = !wifi_ok;

    lv_label_set_text(ui_lblNetName, "");
    lv_label_set_text(ui_lblTime, "");
    lv_label_set_text(ui_lblDate, "");

    // Si entrás directo a PROGRAMADO, dejalo coherente
    programado_apply_mode(g_interval_is_dist);
    programado_refresh_labels();

    lv_obj_add_event_cb(ui_btnServiceAceptar, service_aceptar, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_btnServicePostergar, service_postergar, LV_EVENT_CLICKED, NULL);

    if (!s_ota_callbacks_bound) {
      ota_local_start();
      if (ui_btnOtaCheck) lv_obj_add_flag(ui_btnOtaCheck, LV_OBJ_FLAG_HIDDEN);
      s_ota_callbacks_bound = true;
    }

    service_popup_hide();
  }
  

  // HOME: dotOta visible solo cuando hay update
  if (ui_dotOta) {
    if (g_ota_available) lv_obj_clear_flag(ui_dotOta, LV_OBJ_FLAG_HIDDEN);
    else                 lv_obj_add_flag(ui_dotOta, LV_OBJ_FLAG_HIDDEN);
  }

  // OTA auto-check al boot para que dotOta aparezca en HOME sin entrar a CONFIG
  if (!s_ota_checked_this_boot && home_active && wifi_ok && !g_ota_check_running) {
    ESP_LOGI(TAG, "Boot HOME -> OTA auto-check");
    s_ota_checked_this_boot = true;
    ota_check_async();
  }

  // CONFIG: todo el texto OTA en lblOtaNotes
  if (ui_lblOtaNotes) {
    char ota_text[256];
    if (g_ota_active) {
      strlcpy(ota_text, g_ota_status, sizeof(ota_text));
    } else if (g_ota_available) {
      if (g_ota_notes[0] != '\0') {
        strlcpy(ota_text, g_ota_status, sizeof(ota_text));
        strlcat(ota_text, "\n", sizeof(ota_text));
        strlcat(ota_text, g_ota_notes, sizeof(ota_text));
      } else {
        strlcpy(ota_text, g_ota_status, sizeof(ota_text));
      }
    } else {
      strlcpy(ota_text, (g_ota_status[0] != '\0') ? g_ota_status : "Al dia", sizeof(ota_text));
    }
    lv_label_set_text(ui_lblOtaNotes, ota_text);
  }

  // Si existe label legado, lo dejamos vacío para no duplicar mensajes
  if (ui_lblOtaStatus) lv_label_set_text(ui_lblOtaStatus, "");

  // CONFIG: progreso
  if (ui_barOta) {
    if (g_ota_active) {
      lv_obj_clear_flag(ui_barOta, LV_OBJ_FLAG_HIDDEN);
      lv_bar_set_value(ui_barOta, g_ota_progress, LV_ANIM_OFF);
    } else {
      lv_bar_set_value(ui_barOta, 0, LV_ANIM_OFF);
    }
  }

  // CONFIG: botón actualizar (btnOtaUpdate)
  if (ui_btnOtaUpdate) {
    if (g_ota_available && !g_ota_active) lv_obj_clear_state(ui_btnOtaUpdate, LV_STATE_DISABLED);
    else                                  lv_obj_add_state(ui_btnOtaUpdate, LV_STATE_DISABLED);
  }

  // ============================================================
  // ------------- CONSUMIR REQUEST FLAGS (UI marks) -------------
  // ============================================================
  uint32_t req = g_ui_req_flags;
  if(req) g_ui_req_flags = 0;

   // ============================================================
  // ---------------- SERVICE BUTTONS (UI side) ------------------
  // ============================================================
  if(req & UI_REQ_SERVICE_ACCEPT) {
    time_t now = time(nullptr);

    if(net_time_valid()) {
      g_service.onAccept(now);   // resetea a 45 días
    } else {
      // sin hora válida: no se puede reprogramar, queda postergado
      g_service.onPostpone();
    }

    service_popup_hide(); // LVGL acá OK
  }

  if(req & UI_REQ_SERVICE_POSTPONE) {
    g_service.onPostpone();
    service_popup_hide(); // LVGL acá OK
  }

  // --- WIFI: scan al entrar ---
  if(req & UI_REQ_WIFI_SCAN) {
    cfg_need_scan = true;
    snprintf(cfg_info, sizeof(cfg_info), "Buscando redes...");
    ESP_LOGI(TAG, "UI_REQ_WIFI_SCAN: cfg_need_scan=1 habConfig=%d", (int)habConfig);
  }

  // --- WIFI: toggle (leer LVGL acá, no en events) ---
  if(req & UI_REQ_WIFI_TOGGLE) {
    ESP_LOGI(TAG, "UI_REQ_WIFI_TOGGLE: wifi_ok=%d state=%d ssid='%s' ip='%s'", (int)wifi_ok, (int)cfg_wifi_state, cfg_ssid, cfg_ip);
    if(wifi_ok) {
      snprintf(cfg_info, sizeof(cfg_info), "Desconectando...");
      cfg_do_disconnect = true;
      ESP_LOGI(TAG, "UI toggle -> disconnect request");
    } else {
      const char *pass = (ui_passArea) ? lv_textarea_get_text(ui_passArea) : "";

      char ssid[33] = {0};
      if(ui_Dropdown1) lv_dropdown_get_selected_str(ui_Dropdown1, ssid, sizeof(ssid));

      if(strcmp(ssid, "SIN REDES") == 0 || strlen(ssid) == 0) {
        snprintf(cfg_info, sizeof(cfg_info), "Selecciona una red");
        ESP_LOGW(TAG, "UI toggle -> connect blocked: invalid ssid='%s'", ssid);
      } else {
        strlcpy(pending_ssid, ssid, sizeof(pending_ssid));
        strlcpy(pending_pass, pass, sizeof(pending_pass));
        snprintf(cfg_info, sizeof(cfg_info), "Conectando a %s...", pending_ssid);
        cfg_do_connect = true;
        ESP_LOGI(TAG, "UI toggle -> connect request: ssid='%s' pass_len=%d", pending_ssid, (int)strlen(pending_pass));
      }
    }
  }

  // --- PROGRAMADO ---
  if(req & UI_REQ_PROG_MODE_REFRESH) {
    programado_apply_mode(g_interval_is_dist);
  }
  if(req & UI_REQ_PROG_REFRESH) {
    programado_refresh_labels();
  }

  // --------- visibilidad ----------
  if(s.forceBarsVisible) {
    lv_obj_clear_flag(ui_Group_Settings_Down, LV_OBJ_FLAG_HIDDEN);
  }
  else {
    if(s.headerHidden != last_headerHidden) {
      last_headerHidden = s.headerHidden;
    }

    if(s.settingsHidden != last_settingsHidden) {
      last_settingsHidden = s.settingsHidden;
      if(s.settingsHidden) lv_obj_add_flag(ui_Group_Settings_Down, LV_OBJ_FLAG_HIDDEN);
      else                 lv_obj_clear_flag(ui_Group_Settings_Down, LV_OBJ_FLAG_HIDDEN);
    }

    if(s.showReinitBtn) lv_obj_clear_flag(ui_groupReinitbtn, LV_OBJ_FLAG_HIDDEN);
    else                lv_obj_add_flag(ui_groupReinitbtn, LV_OBJ_FLAG_HIDDEN);
  }

  // --------- hora/fecha y SSID WIFI----------
  if(wifi_ok) {
    if(strcmp(ssid_l, last_cfg_ssid) != 0) {
      strlcpy(last_cfg_ssid, ssid_l, sizeof(last_cfg_ssid));
      lv_label_set_text(ui_lblNetName, ssid_l);
    }
  } else {
    if(last_cfg_ssid[0] != '\0') {
      last_cfg_ssid[0] = '\0';
      lv_label_set_text(ui_lblNetName, "");
    }
  }

  ui_net_time_update(s);
  if (!wifi_ok) {
    s.showNetTime = false;
  }

  // ============================================================
  // --------- SERVICE 45 DIAS (solo boot + solo HOME) ----------
  // ============================================================
  const bool is_home = home_active;

  // 1) Evaluar vencimiento SOLO 1 vez por boot, SOLO si:
  //    - estamos en HOME
  //    - hay hora válida (NTP)
  if(!s_service_evaluated_this_boot && is_home && net_time_valid()) {
    time_t now = time(nullptr);
    g_service.evaluate_on_boot(now);   // si venció -> deja pending=true
    s_service_evaluated_this_boot = true;
  }

  // 2) Mostrar popup SOLO 1 vez por boot y SOLO en HOME
  if(!s_service_popup_attempted_this_boot && is_home) {
    if(g_service.pending()) {
      service_popup_update_text();
      service_popup_show();
    } else {
      service_popup_hide();
    }
    s_service_popup_attempted_this_boot = true;
  }


  if(s.showNetTime) {
    if(strcmp(s.timeStr, last_timeStr) != 0) {
      strlcpy(last_timeStr, s.timeStr, sizeof(last_timeStr));
      lv_label_set_text(ui_lblTime, s.timeStr);
    }
    if(strcmp(s.dateStr, last_dateStr) != 0) {
      strlcpy(last_dateStr, s.dateStr, sizeof(last_dateStr));
      lv_label_set_text(ui_lblDate, s.dateStr);
    }
  } else {
    if(last_timeStr[0] != '\0') {
      last_timeStr[0] = '\0';
      lv_label_set_text(ui_lblTime, "");
    }
    if(last_dateStr[0] != '\0') {
      last_dateStr[0] = '\0';
      lv_label_set_text(ui_lblDate, "");
    }
  }

  // --------- velocidad + cursor ----------
  char speed_to_show[16];

  if(g_workout_frozen) {
    snprintf(speed_to_show, sizeof(speed_to_show), "%.1f", (double)g_workout_final.avg_speed_kmh);
  } else {
    strlcpy(speed_to_show, s.speedStr, sizeof(speed_to_show));
  }

  if(speed_to_show[0] != '\0' && strcmp(speed_to_show, last_speedStr) != 0) {
    strlcpy(last_speedStr, speed_to_show, sizeof(last_speedStr));
    lv_label_set_text(ui_lblSpeedNumber, speed_to_show);

    if(s.cursorAngle != last_cursorAngle) {
      last_cursorAngle = s.cursorAngle;
      lv_img_set_angle(ui_imgCursorSpeed, s.cursorAngle);
    }
  }

  // --------- ritmo ----------
  char ritmo_to_show[16];

  if(g_workout_frozen) {
    float pace = g_workout_final.avg_pace_min_km;
    if(pace < 0) pace = 0;

    int mm = (int)pace;
    int ss = (int)((pace - mm) * 60.0f + 0.5f);
    if(ss >= 60) { ss -= 60; mm++; }

    snprintf(ritmo_to_show, sizeof(ritmo_to_show), "%02d:%02d", mm, ss);
  } else {
    strlcpy(ritmo_to_show, s.ritmoStr, sizeof(ritmo_to_show));
  }

  if(ritmo_to_show[0] != '\0' && strcmp(ritmo_to_show, last_ritmoStr) != 0) {
    strlcpy(last_ritmoStr, ritmo_to_show, sizeof(last_ritmoStr));
    lv_label_set_text(ui_LabelVarRitm, ritmo_to_show);
  }

  //---------ARO RGB-------------
  ring_apply_color(g_rgb_target_r, g_rgb_target_g, g_rgb_target_b);

  // --------- distancia ----------
  char dist_to_show[16];

  if(g_workout_frozen) {
    snprintf(dist_to_show, sizeof(dist_to_show), "%.0f", (double)g_workout_final.total_dist_m);
  } else {
    strlcpy(dist_to_show, s.distStr, sizeof(dist_to_show));
  }

  if(strcmp(dist_to_show, last_distStr) != 0) {
    strlcpy(last_distStr, dist_to_show, sizeof(last_distStr));
    lv_label_set_text(ui_lblDistanceNumber, dist_to_show);
  }

  // --------- unidad ----------
  if(strcmp(s.distUnit, last_distUnit) != 0) {
    strlcpy(last_distUnit, s.distUnit, sizeof(last_distUnit));
    lv_label_set_text(ui_lblDistanceUnit, s.distUnit);
  }

  // --------- calorías ----------
  char cal_to_show[16];

  if(g_workout_frozen) {
    snprintf(cal_to_show, sizeof(cal_to_show), "%.0f", (double)g_workout_final.total_kcal);
  } else {
    strlcpy(cal_to_show, s.calStr, sizeof(cal_to_show));
  }

  if(strcmp(cal_to_show, last_calStr) != 0) {
    strlcpy(last_calStr, cal_to_show, sizeof(last_calStr));
    lv_label_set_text(ui_lblCaloriasNumber, cal_to_show);
  }

  // --------- tiempo / setpoint (ui_lblTimeNumber) ----------
  char time_to_show[16];

  if(g_workout_frozen) {
    uint32_t total_s = g_workout_final.total_time_ms / 1000;

    uint32_t hh = total_s / 3600;
    uint32_t mm = (total_s % 3600) / 60;
    uint32_t ss = total_s % 60;

    snprintf(time_to_show, sizeof(time_to_show),
            "%02lu:%02lu:%02lu",
            (unsigned long)hh,
            (unsigned long)mm,
            (unsigned long)ss);
  }
  else {
    const char *src = s.trainTimeStr;

    if(s.intervalShowSetpoint && !s.intervalIsDistance && s.intervalSetpointStr[0] != 0) {
      src = s.intervalSetpointStr;
    }

    strlcpy(time_to_show, src, sizeof(time_to_show));
  }

  if(strcmp(time_to_show, last_trainTimeStr) != 0) {
    strlcpy(last_trainTimeStr, time_to_show, sizeof(last_trainTimeStr));
    lv_label_set_text(ui_lblTimeNumber, time_to_show);
  }

  // --------- pausa (cartel único) ----------
  if(s.pausaVisible != last_pausaVisible) {
    last_pausaVisible = s.pausaVisible;
    if(s.pausaVisible) lv_obj_clear_flag(ui_groupPausa, LV_OBJ_FLAG_HIDDEN);
    else               lv_obj_add_flag(ui_groupPausa, LV_OBJ_FLAG_HIDDEN);
  }

  if(s.pausaVisible) {

    if(s.pausaIsSerie != last_pausaIsSerie) {
      last_pausaIsSerie = s.pausaIsSerie;
      lv_color_t c = s.pausaIsSerie ? lv_color_hex(0xCC2E2E) : lv_color_hex(0xE0C100);
      lv_obj_set_style_bg_color(ui_groupPausa, c, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if(strcmp(s.pausaTitle, last_pausaTitle) != 0) {
      strlcpy(last_pausaTitle, s.pausaTitle, sizeof(last_pausaTitle));
      lv_label_set_text(ui_lblTittlePausa, s.pausaTitle);
    }

    if(strcmp(s.pausaStr, last_pausaStr) != 0) {
      strlcpy(last_pausaStr, s.pausaStr, sizeof(last_pausaStr));
      lv_label_set_text(ui_lblPausa, s.pausaStr);
    }
  }

  // --------- pantalla config ----------
  if(config_active != last_habConfig) {
    last_habConfig = config_active;

    last_cfg_info[0] = '\0';
    last_cfg_ip[0]   = '\0';
    last_cfg_ssid[0] = '\0';
    last_wifi_ok     = !wifi_ok;

    wifi_btn_apply(wifi_ok);
    last_btn_checked = wifi_ok;
    btn_checked_inited = true;

    if (config_active && ui_passArea) {
      if (pending_pass[0] != '\0') lv_textarea_set_text(ui_passArea, "********");
      else                          lv_textarea_set_text(ui_passArea, "");
    }

    if (config_active && ui_Dropdown1) {
      lv_dropdown_set_options(ui_Dropdown1, "BUSCANDO REDES...");
      lv_dropdown_set_selected(ui_Dropdown1, 0);
      cfg_need_scan = true;

      if (wifi_ok && !g_ota_check_running) {
        ESP_LOGI(TAG, "Entering CONFIG -> OTA auto-check");
        ota_check_async();
      }
    }
  }

  if(config_active) {
    // --------- Service (dias restantes / postergado) ----------
    static char last_serviceStr[32] = "";

    char svcStr[32];
    time_t now = time(nullptr);

    g_service.formatConfigLabel(svcStr, sizeof(svcStr), now, net_time_valid());

    if(strcmp(svcStr, last_serviceStr) != 0) {
      strlcpy(last_serviceStr, svcStr, sizeof(last_serviceStr));
      lv_label_set_text(ui_lblOdoMaquina, svcStr);
    }

    if(strcmp(info_l, last_cfg_info) != 0) {
      strlcpy(last_cfg_info, info_l, sizeof(last_cfg_info));
      lv_label_set_text(ui_lblInfo, info_l);
    }

    const char *ip_raw = (ip_l[0] != '\0') ? ip_l : "0.0.0.0";
    char ip_show[32];
    snprintf(ip_show, sizeof(ip_show), "IP: %s", ip_raw);
    if(strcmp(ip_show, last_cfg_ip) != 0) {
      strlcpy(last_cfg_ip, ip_show, sizeof(last_cfg_ip));
      lv_label_set_text(ui_lblNetworkIP, ip_show);
    }

    wifi_icons_apply(wifi_ok);

    if (ui_btnWifi) {
      bool btn_checked = lv_obj_has_state(ui_btnWifi, LV_STATE_CHECKED);
      if (!btn_checked_inited) {
        last_btn_checked = btn_checked;
        btn_checked_inited = true;
      } else if (btn_checked != last_btn_checked) {
        last_btn_checked = btn_checked;
        ESP_LOGI(TAG, "btnWifi toggled (polled): checked=%d wifi_ok=%d state=%d", (int)btn_checked, (int)wifi_ok, (int)cfg_wifi_state);

        if (btn_checked && !wifi_ok) {
          const char *pass = (ui_passArea) ? lv_textarea_get_text(ui_passArea) : "";
          char ssid[33] = {0};
          if (ui_Dropdown1) lv_dropdown_get_selected_str(ui_Dropdown1, ssid, sizeof(ssid));

          if(strcmp(ssid, "SIN REDES") == 0 || strcmp(ssid, "BUSCANDO REDES...") == 0 || strlen(ssid) == 0) {
            snprintf(cfg_info, sizeof(cfg_info), "Selecciona una red");
            ESP_LOGW(TAG, "polled connect blocked: invalid ssid='%s'", ssid);
            wifi_btn_apply(false);
            last_btn_checked = false;
          } else {
            strlcpy(pending_ssid, ssid, sizeof(pending_ssid));

            if (!(is_masked_password(pass) && pending_pass[0] != '\0')) {
              strlcpy(pending_pass, pass, sizeof(pending_pass));
            }

            snprintf(cfg_info, sizeof(cfg_info), "Conectando a %s...", pending_ssid);
            cfg_do_connect = true;
            ESP_LOGI(TAG, "polled connect request: ssid='%s' pass_len=%d", pending_ssid, (int)strlen(pending_pass));
          }
        } else if (!btn_checked && wifi_ok) {
          snprintf(cfg_info, sizeof(cfg_info), "Desconectando...");
          cfg_do_disconnect = true;
          ESP_LOGI(TAG, "polled disconnect request");
        }
      }
    }

    if(cfg_scan_ready || cfg_dd_opts_ready) {
      cfg_scan_ready = false;
      cfg_dd_opts_ready = false;
      lv_dropdown_set_options(ui_Dropdown1, cfg_dd_opts);

      if (pending_ssid[0] != '\0') {
        int16_t idx = lv_dropdown_get_option_index(ui_Dropdown1, pending_ssid);
        if (idx >= 0) lv_dropdown_set_selected(ui_Dropdown1, (uint16_t)idx);
        else          lv_dropdown_set_selected(ui_Dropdown1, 0);
      } else {
        lv_dropdown_set_selected(ui_Dropdown1, 0);
      }
    }

    char mac_show[40];
    snprintf(mac_show, sizeof(mac_show), "MAC: %s", (cfg_mac[0] != '\0') ? cfg_mac : "--:--:--:--:--:--");
    const char *fw_show = (cfg_fw[0] != '\0') ? cfg_fw : (g_fw_version ? g_fw_version : "0.0.0");
    lv_label_set_text(ui_lblMac, mac_show);
    lv_label_set_text(ui_lblFirmwareVersion, fw_show);

    bool busy = (cfg_wifi_state == CFG_WIFI_CONNECTING);
    if(busy) lv_obj_add_state(ui_btnWifi, LV_STATE_DISABLED);
    else     lv_obj_clear_state(ui_btnWifi, LV_STATE_DISABLED);
  }

  // --------- iconos wifi ----------
  if(wifi_ok != last_wifi_ok) {
    last_wifi_ok = wifi_ok;
    ESP_LOGI(TAG, "wifi_ok changed -> %d (state=%d, ssid='%s', ip='%s')", (int)wifi_ok, (int)cfg_wifi_state, cfg_ssid, cfg_ip);
    wifi_icons_apply(wifi_ok);
    wifi_btn_apply(wifi_ok);
    last_btn_checked = wifi_ok;
    btn_checked_inited = true;
  }

  // --------- intervalado: series/pasadas ----------
  if(strcmp(s.intervalSeriesStr, last_intervalSeriesStr) != 0) {
    strlcpy(last_intervalSeriesStr, s.intervalSeriesStr, sizeof(last_intervalSeriesStr));
    lv_label_set_text(ui_lblNumSeries, s.intervalSeriesStr);
  }
  if(strcmp(s.intervalPasadasStr, last_intervalPasadasStr) != 0) {
    strlcpy(last_intervalPasadasStr, s.intervalPasadasStr, sizeof(last_intervalPasadasStr));
    lv_label_set_text(ui_lblNumPasadas, s.intervalPasadasStr);
  }
}

extern "C" void ui_refresh_start_timer(void)
{
  static lv_timer_t *s_ui_refresh_timer = NULL;
  if (s_ui_refresh_timer == NULL) {
    s_ui_refresh_timer = lv_timer_create(ui_refresh_cb, 100, NULL);
  }
}
