#include <string.h>
#include <stdio.h>

#include "wifi_mgr.h"
#include "app_globals.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"
#include "esp_sntp.h"

#include "lwip/inet.h"
#include <stdlib.h>
#include <time.h>

static const char *TAG = "wifi_mgr";

static TaskHandle_t s_task = NULL;
static bool s_inited = false;

// conexión no bloqueante
static uint32_t s_connect_t0 = 0;
static const uint32_t CONNECT_TIMEOUT_MS = 15000;

// scan async
static bool s_scan_running = false;

// mantenimiento fuera de config
static uint32_t s_reconn_t0 = 0;
static const uint32_t RECONN_EVERY_MS = 5000;
static bool s_allow_reconnect = false;

// SNTP (en IDF podés dejarlo para tu módulo de hora; acá solo marcamos habLocalTime si querés)
static bool s_sntp_started = false;
static const char *LOCAL_TZ = "UTC+3";

static void refresh_cfg_mac_fw(void)
{
  uint8_t mac[6] = {0};
  bool mac_ok = false;

  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif && esp_netif_get_mac(netif, mac) == ESP_OK) {
    mac_ok = true;
  }

  if (!mac_ok && esp_base_mac_addr_get(mac) == ESP_OK) {
    mac_ok = true;
  }

  if (mac_ok) {
    snprintf(cfg_mac, sizeof(cfg_mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else {
    strlcpy(cfg_mac, "--:--:--:--:--:--", sizeof(cfg_mac));
  }

  strlcpy(cfg_fw, g_fw_version ? g_fw_version : "?", sizeof(cfg_fw));
}

// --- helpers tiempo ---
static inline uint32_t now_ms(void) {
  return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

// ✅ ÚNICA fuente de verdad hacia tus globals
static void set_globals_disconnected(void)
{
  wifi_ok = false;
  strlcpy(cfg_ssid, "SIN RED", sizeof(cfg_ssid));
  strlcpy(cfg_ip,   "0.0.0.0", sizeof(cfg_ip));
}

static void set_globals_connected_from_netif(void)
{
  // SSID
  wifi_ap_record_t ap = {0};
  if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
    strlcpy(cfg_ssid, (const char*)ap.ssid, sizeof(cfg_ssid));
  }

  // IP
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif) {
    esp_netif_ip_info_t ip;
    if (esp_netif_get_ip_info(netif, &ip) == ESP_OK) {
      char ipbuf[16];
      esp_ip4addr_ntoa(&ip.ip, ipbuf, sizeof(ipbuf));
      strlcpy(cfg_ip, ipbuf, sizeof(cfg_ip));
    }
  }

  wifi_ok = true;

  // Si querés, acá arrancás SNTP IDF (o lo dejás para otro módulo)
  if (!s_sntp_started) {
    s_sntp_started = true;
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    setenv("TZ", LOCAL_TZ, 1);
    tzset();
    habLocalTime = true;
  }
}

// ---- Scan dropdown (igual que tu build_dropdown_opts_from_scan) ----
static void build_dropdown_opts_from_scan(const wifi_ap_record_t *recs, int n)
{
  cfg_dd_opts[0] = '\0';
  scan_n = 0;

  int count = (n > MAX_NETS) ? MAX_NETS : n;
  scan_n = count;

  for (int i = 0; i < count; i++) {
    const char *s = (const char*)recs[i].ssid;
    if (!s || !s[0]) continue;

    strlcpy(scan_ssid[i], s, sizeof(scan_ssid[i]));

    if (strlen(cfg_dd_opts) + strlen(s) + 2 < sizeof(cfg_dd_opts)) {
      strlcat(cfg_dd_opts, s, sizeof(cfg_dd_opts));
      strlcat(cfg_dd_opts, "\n", sizeof(cfg_dd_opts));
    }
  }

  if (scan_n <= 0 || cfg_dd_opts[0] == '\0') {
    strlcpy(cfg_dd_opts, "SIN REDES", sizeof(cfg_dd_opts));
  }
}

// ---- Event handler ----
static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
  if (base == WIFI_EVENT) {

    if (id == WIFI_EVENT_STA_DISCONNECTED) {
      // Se cayó
      set_globals_disconnected();
      ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED: state=%d", (int)cfg_wifi_state);

      // Si estabas conectando manualmente, la task manejará timeout y estado
      if (cfg_wifi_state == CFG_WIFI_CONNECTING) {
        // dejamos que la task decida mensaje, acá solo marcamos
      }

    } else if (id == WIFI_EVENT_SCAN_DONE) {
      s_scan_running = false;

      uint16_t ap_count = 0;
      esp_wifi_scan_get_ap_num(&ap_count);

      if (ap_count == 0) {
        strlcpy(cfg_dd_opts, "SIN REDES", sizeof(cfg_dd_opts));
        scan_n = 0;
      } else {
        wifi_ap_record_t *recs = (wifi_ap_record_t *)heap_caps_malloc(sizeof(wifi_ap_record_t) * ap_count, MALLOC_CAP_8BIT);
        if (recs) {
          uint16_t n = ap_count;
          esp_wifi_scan_get_ap_records(&n, recs);
          build_dropdown_opts_from_scan(recs, n);
          heap_caps_free(recs);
        } else {
          strlcpy(cfg_dd_opts, "SIN REDES", sizeof(cfg_dd_opts));
          scan_n = 0;
        }
      }

      if (scan_n > 0) snprintf(cfg_info, sizeof(cfg_info), "Redes: %d", scan_n);
      else            snprintf(cfg_info, sizeof(cfg_info), "No hay redes disponibles");

      ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE: ap_count=%u scan_n=%d opts_len=%u", (unsigned)ap_count, scan_n, (unsigned)strlen(cfg_dd_opts));

      cfg_scan_ready = true;
      cfg_dd_opts_ready = true;
      cfg_wifi_state = CFG_WIFI_SCAN_DONE;
    }

  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    // Conectó
    set_globals_connected_from_netif();

    if (data) {
      ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
      char ipbuf[16];
      esp_ip4addr_ntoa(&evt->ip_info.ip, ipbuf, sizeof(ipbuf));
      strlcpy(cfg_ip, ipbuf, sizeof(cfg_ip));
    }

    snprintf(cfg_info, sizeof(cfg_info), "Conectado a %s", cfg_ssid);
    cfg_wifi_state = CFG_WIFI_CONNECTED;
    ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP: ssid='%s' ip='%s'", cfg_ssid, cfg_ip);
  }
}

esp_err_t wifi_mgr_init(void)
{
  if (s_inited) return ESP_OK;

  // NVS requerido por WiFi
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  } else {
    ESP_ERROR_CHECK(err);
  }

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // STA default netif
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  // MAC + FW en globals (como tu enter_config)
  refresh_cfg_mac_fw();

  set_globals_disconnected();
  cfg_wifi_state = CFG_WIFI_IDLE;

  s_inited = true;
  ESP_LOGI(TAG, "init OK");
  return ESP_OK;
}

void wifi_mgr_on_enter_config(void)
{
  refresh_cfg_mac_fw();

  wifi_config_t wc = {0};
  if (esp_wifi_get_config(WIFI_IF_STA, &wc) == ESP_OK) {
    if (wc.sta.ssid[0] != '\0') {
      strlcpy(pending_ssid, (const char*)wc.sta.ssid, sizeof(pending_ssid));
    } else {
      pending_ssid[0] = '\0';
    }

    if (wc.sta.password[0] != '\0') {
      strlcpy(pending_pass, (const char*)wc.sta.password, sizeof(pending_pass));
    } else {
      pending_pass[0] = '\0';
    }
  }

  cfg_scan_ready = false;
  cfg_dd_opts_ready = false;
  strlcpy(cfg_dd_opts, "BUSCANDO REDES...", sizeof(cfg_dd_opts));
  scan_n = 0;

  // Mensaje inicial
  if (wifi_ok) snprintf(cfg_info, sizeof(cfg_info), "Conectado a %s", cfg_ssid);
  else         snprintf(cfg_info, sizeof(cfg_info), "Desconectado");

  cfg_need_scan = true;
}

void wifi_mgr_on_exit_config(void)
{
  cfg_wifi_state = CFG_WIFI_IDLE;
}

// Reconexión suave fuera de config (usa credenciales guardadas por el stack)
static void wifi_maintenance(void)
{
  if (wifi_ok) return;
  if (!s_allow_reconnect) return;

  uint32_t now = now_ms();
  if (now - s_reconn_t0 < RECONN_EVERY_MS) return;
  s_reconn_t0 = now;

  // Reintento suave
  esp_wifi_connect();
}

// --- task: reemplaza wifi_mgr_loop() ---
static void wifi_mgr_task(void *pv)
{
  for (;;) {

    // Entró a config (1 vez)
    if (cfg_entered) {
      cfg_entered = false;
      wifi_mgr_on_enter_config();

      if (wifi_ok) cfg_wifi_state = CFG_WIFI_CONNECTED;
      else         cfg_wifi_state = CFG_WIFI_IDLE;
    }

    // Si NO estás en config: mantenimiento + apagar server si estaba
    if (!habConfig) {
      wifi_maintenance();
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    // ------------------- DISCONNECT (manual) -------------------
    if (cfg_do_disconnect) {
      cfg_do_disconnect = false;

      ESP_LOGI(TAG, "cfg_do_disconnect=1 -> esp_wifi_disconnect()");

      esp_wifi_disconnect();
      s_allow_reconnect = false;
      snprintf(cfg_info, sizeof(cfg_info), "Desconectado");
      cfg_wifi_state = CFG_WIFI_IDLE;
      set_globals_disconnected();
    }

    // ------------------- START SCAN -------------------
    if (cfg_need_scan && !s_scan_running) {
      cfg_need_scan = false;

      ESP_LOGI(TAG, "cfg_need_scan=1 -> start scan");

      cfg_wifi_state = CFG_WIFI_SCANNING;
      snprintf(cfg_info, sizeof(cfg_info), "Buscando redes...");

      cfg_scan_ready = false;
      cfg_dd_opts_ready = false;
      scan_n = 0;
      cfg_dd_opts[0] = '\0';

      wifi_scan_config_t sc = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE
      };

      s_scan_running = true;
      esp_err_t err = esp_wifi_scan_start(&sc, false); // async
      if (err != ESP_OK) {
        s_scan_running = false;
        cfg_wifi_state = CFG_WIFI_FAIL;
        snprintf(cfg_info, sizeof(cfg_info), "Scan fallo (%s)", esp_err_to_name(err));
        ESP_LOGW(TAG, "esp_wifi_scan_start failed: %s", esp_err_to_name(err));
      }
    }

    // ------------------- CONNECT START -------------------
    if (cfg_do_connect) {
      cfg_do_connect = false;
      s_allow_reconnect = true;

      ESP_LOGI(TAG, "cfg_do_connect=1: pending_ssid='%s' pass_len=%d", pending_ssid, (int)strlen(pending_pass));

      if (strlen(pending_ssid) == 0 || strcmp(pending_ssid, "SIN REDES") == 0) {
        snprintf(cfg_info, sizeof(cfg_info), "Selecciona una red");
      } else {
        snprintf(cfg_info, sizeof(cfg_info), "Conectando a %s...", pending_ssid);

        wifi_config_t wc = {0};
        strlcpy((char*)wc.sta.ssid, pending_ssid, sizeof(wc.sta.ssid));
        strlcpy((char*)wc.sta.password, pending_pass, sizeof(wc.sta.password));
        wc.sta.pmf_cfg.capable = true;
        wc.sta.pmf_cfg.required = false;

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));

        cfg_wifi_state = CFG_WIFI_CONNECTING;
        s_connect_t0 = now_ms();

        // Limpia estado anterior y conecta
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(50));
        ESP_ERROR_CHECK(esp_wifi_connect());
        ESP_LOGI(TAG, "esp_wifi_connect() requested");
      }
    }

    // ------------------- CONNECT PROGRESS (timeout / mensajes) -------------------
    if (cfg_wifi_state == CFG_WIFI_CONNECTING && !wifi_ok) {
      uint32_t elapsed = now_ms() - s_connect_t0;

      if (elapsed > CONNECT_TIMEOUT_MS) {
        snprintf(cfg_info, sizeof(cfg_info), "Fallo al conectar (timeout)");
        esp_wifi_disconnect();
        cfg_wifi_state = CFG_WIFI_FAIL;
      } else {
        // IDF no da WL_* directo; dejamos un texto genérico con contador
        snprintf(cfg_info, sizeof(cfg_info), "Conectando... (%lus)", (unsigned long)(elapsed/1000));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void wifi_mgr_start_task(void)
{
  if (!s_task) {
    xTaskCreatePinnedToCore(wifi_mgr_task, "wifi_mgr", 6144, NULL, 8, &s_task, 0);
  }
}

void wifi_mgr_stop(void)
{
  if (s_task) {
    vTaskDelete(s_task);
    s_task = NULL;
  }
}