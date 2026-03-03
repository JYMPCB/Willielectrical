#include "ota_mgr.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_crt_bundle.h"
#include "esp_wifi.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "cJSON.h"

#include "app_globals.h"
#include "data.h"

// ---------- Config ----------
static const uint32_t OTA_HTTP_TIMEOUT_MS = 15000;
static const uint32_t OTA_HTTP_STALL_TIMEOUT_MS = 30000;
static volatile bool s_cancel_req = false;
static const char *TAG = "ota_mgr";
static httpd_handle_t s_httpd = NULL;
static esp_timer_handle_t s_reboot_timer = NULL;

static void set_status(const char* s);

typedef struct {
  char *data;
  size_t len;
  size_t cap;
} http_buffer_t;

static esp_err_t http_collect_cb(esp_http_client_event_t *evt)
{
  if (evt->event_id != HTTP_EVENT_ON_DATA || evt->data_len <= 0 || !evt->user_data) {
    return ESP_OK;
  }

  http_buffer_t *buf = (http_buffer_t *)evt->user_data;
  size_t needed = buf->len + (size_t)evt->data_len + 1;
  if (needed > buf->cap) {
    size_t new_cap = (buf->cap == 0) ? 512 : buf->cap;
    while (new_cap < needed) new_cap *= 2;
    char *new_data = (char *)realloc(buf->data, new_cap);
    if (!new_data) {
      return ESP_ERR_NO_MEM;
    }
    buf->data = new_data;
    buf->cap = new_cap;
  }

  memcpy(buf->data + buf->len, evt->data, (size_t)evt->data_len);
  buf->len += (size_t)evt->data_len;
  buf->data[buf->len] = '\0';

  return ESP_OK;
}

static void ota_apply_auth_headers(esp_http_client_handle_t client)
{
  if (!client) return;

  if (OTA_AUTH_HEADER_NAME[0] != '\0' && OTA_AUTH_HEADER_VALUE[0] != '\0') {
    esp_http_client_set_header(client, OTA_AUTH_HEADER_NAME, OTA_AUTH_HEADER_VALUE);
  }
}

static bool ota_network_ready(void)
{
  wifi_ap_record_t ap = {};
  return esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
}

static void ota_reboot_timer_cb(void *arg)
{
  (void)arg;
  esp_restart();
}

static void ota_schedule_reboot_ms(uint32_t delay_ms)
{
  if (s_reboot_timer == NULL) {
    esp_timer_create_args_t args = {};
    args.callback = &ota_reboot_timer_cb;
    args.name = "ota_reboot";
    if (esp_timer_create(&args, &s_reboot_timer) != ESP_OK) {
      return;
    }
  }

  esp_timer_stop(s_reboot_timer);
  esp_timer_start_once(s_reboot_timer, (uint64_t)delay_ms * 1000ULL);
}

static esp_err_t local_root_get_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");
  return httpd_resp_send(req, Pagina, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t local_info_get_handler(httpd_req_t *req)
{
  char body[160];
  snprintf(body, sizeof(body),
           "{\"fw\":\"%s\",\"ip\":\"%s\",\"ota_active\":%s,\"ota_status\":\"%s\"}",
           g_fw_version ? g_fw_version : "?",
           cfg_ip,
           g_ota_active ? "true" : "false",
           g_ota_status);

  httpd_resp_set_type(req, "application/json; charset=utf-8");
  return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t local_ota_post_handler(httpd_req_t *req)
{
  if (!ota_network_ready()) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    return httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"Sin WiFi\"}");
  }

  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
  if (!update_partition) {
    httpd_resp_set_status(req, "500 Internal Server Error");
    return httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"Particion OTA invalida\"}");
  }

  esp_ota_handle_t ota_handle = 0;
  esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
  if (err != ESP_OK) {
    httpd_resp_set_status(req, "500 Internal Server Error");
    return httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"esp_ota_begin fail\"}");
  }

  g_ota_active = true;
  g_ota_progress = 0;
  set_status("Cargando...");

  int remaining = req->content_len;
  int total = req->content_len;
  uint8_t buffer[1024];

  while (remaining > 0) {
    int to_read = (remaining > (int)sizeof(buffer)) ? (int)sizeof(buffer) : remaining;
    int recv_len = httpd_req_recv(req, (char *)buffer, to_read);
    if (recv_len <= 0) {
      esp_ota_abort(ota_handle);
      g_ota_active = false;
      set_status("OTA fallo");
      httpd_resp_set_status(req, "500 Internal Server Error");
      return httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"Error recibiendo bin\"}");
    }

    err = esp_ota_write(ota_handle, buffer, (size_t)recv_len);
    if (err != ESP_OK) {
      esp_ota_abort(ota_handle);
      g_ota_active = false;
      set_status("OTA fallo");
      httpd_resp_set_status(req, "500 Internal Server Error");
      return httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"Error escribiendo OTA\"}");
    }

    remaining -= recv_len;
    if (total > 0) {
      int p = ((total - remaining) * 100) / total;
      if (p < 0) p = 0;
      if (p > 100) p = 100;
      g_ota_progress = p;
    }
  }

  err = esp_ota_end(ota_handle);
  if (err == ESP_OK) {
    err = esp_ota_set_boot_partition(update_partition);
  }

  if (err != ESP_OK) {
    g_ota_active = false;
    set_status("OTA fallo");
    httpd_resp_set_status(req, "500 Internal Server Error");
    return httpd_resp_sendstr(req, "{\"ok\":false,\"msg\":\"Error finalizando OTA\"}");
  }

  g_ota_progress = 100;
  g_ota_active = false;
  set_status("OK, reiniciando");

  httpd_resp_set_type(req, "application/json; charset=utf-8");
  httpd_resp_sendstr(req, "{\"ok\":true,\"msg\":\"Actualizado con exito\"}");

  ota_schedule_reboot_ms(800);
  return ESP_OK;
}

void ota_local_start()
{
  if (s_httpd) return;

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.recv_wait_timeout = 30;

  if (httpd_start(&s_httpd, &config) != ESP_OK) {
    ESP_LOGW(TAG, "No se pudo iniciar OTA local webserver");
    s_httpd = NULL;
    return;
  }

  httpd_uri_t uri_root = {};
  uri_root.uri = "/";
  uri_root.method = HTTP_GET;
  uri_root.handler = local_root_get_handler;
  httpd_register_uri_handler(s_httpd, &uri_root);

  httpd_uri_t uri_info = {};
  uri_info.uri = "/info";
  uri_info.method = HTTP_GET;
  uri_info.handler = local_info_get_handler;
  httpd_register_uri_handler(s_httpd, &uri_info);

  httpd_uri_t uri_ota = {};
  uri_ota.uri = "/actualizar";
  uri_ota.method = HTTP_POST;
  uri_ota.handler = local_ota_post_handler;
  httpd_register_uri_handler(s_httpd, &uri_ota);

  ESP_LOGI(TAG, "OTA local webserver activo");
}

void ota_local_stop()
{
  if (!s_httpd) return;
  httpd_stop(s_httpd);
  s_httpd = NULL;
}

static int semver_cmp(const char* a, const char* b) {
  // Compara "MAJOR.MINOR.PATCH" (simple y suficiente)
  int a1=0,a2=0,a3=0,b1=0,b2=0,b3=0;
  sscanf(a ? a : "0.0.0", "%d.%d.%d", &a1,&a2,&a3);
  sscanf(b ? b : "0.0.0", "%d.%d.%d", &b1,&b2,&b3);
  if(a1!=b1) return (a1>b1)? 1:-1;
  if(a2!=b2) return (a2>b2)? 1:-1;
  if(a3!=b3) return (a3>b3)? 1:-1;
  return 0;
}

static void set_status(const char* s) {
  if(!s) s = "";
  strlcpy(g_ota_status, s, sizeof(g_ota_status));
}

void ota_request_cancel() {
  s_cancel_req = true;
}

// ---------- OTA CHECK TASK ----------
static void ota_check_task(void* pv) {
  (void)pv;

  if (g_ota_check_running) { vTaskDelete(NULL); return; }
  g_ota_check_running = true;

  if (!ota_network_ready()) {
    set_status("Sin WiFi");
    g_ota_check_running = false;
    vTaskDelete(NULL);
    return;
  }

  set_status("Buscando update...");
  g_ota_available = false;
  g_ota_latest_ver[0] = '\0';
  g_ota_bin_url[0] = '\0';
  g_ota_notes[0] = '\0';

  http_buffer_t body = {};
  esp_http_client_config_t cfg = {};
  cfg.url = OTA_MANIFEST_URL;
  cfg.timeout_ms = (int)OTA_HTTP_TIMEOUT_MS;
  cfg.transport_type = HTTP_TRANSPORT_OVER_SSL;
  cfg.event_handler = http_collect_cb;
  cfg.user_data = &body;
  cfg.crt_bundle_attach = esp_crt_bundle_attach;

  esp_http_client_handle_t client = esp_http_client_init(&cfg);
  if (!client) {
    set_status("Error init manifest");
    g_ota_check_running = false;
    free(body.data);
    vTaskDelete(NULL);
    return;
  }

  ota_apply_auth_headers(client);

  esp_err_t err = esp_http_client_perform(client);
  int code = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  if (err != ESP_OK || code != 200) {
    set_status("Manifest HTTP error");
    g_ota_check_running = false;
    free(body.data);
    vTaskDelete(NULL);
    return;
  }

  cJSON *doc = cJSON_Parse(body.data ? body.data : "");
  if (!doc) {
    set_status("JSON invalido");
    g_ota_check_running = false;
    free(body.data);
    vTaskDelete(NULL);
    return;
  }

  const cJSON *v = cJSON_GetObjectItemCaseSensitive(doc, "version");
  const cJSON *u = cJSON_GetObjectItemCaseSensitive(doc, "bin_url");
  const cJSON *n = cJSON_GetObjectItemCaseSensitive(doc, "notes");

  const char* latest_ver = cJSON_IsString(v) ? v->valuestring : "";
  const char* bin_url    = cJSON_IsString(u) ? u->valuestring : "";
  const char* notes      = cJSON_IsString(n) ? n->valuestring : "";

  if (strlen(latest_ver) == 0 || strlen(bin_url) == 0) {
    set_status("Manifest incompleto");
    cJSON_Delete(doc);
    g_ota_check_running = false;
    free(body.data);
    vTaskDelete(NULL);
    return;
  }

  // Comparo con tu versión local
  if (semver_cmp(latest_ver, g_fw_version) > 0) {
    strlcpy(g_ota_latest_ver, latest_ver, sizeof(g_ota_latest_ver));
    strlcpy(g_ota_bin_url, bin_url, sizeof(g_ota_bin_url));
    strlcpy(g_ota_notes, notes, sizeof(g_ota_notes));
    g_ota_available = true;
    set_status("Update disponible");
  } else {
    set_status("Al dia");
  }

  cJSON_Delete(doc);
  free(body.data);
  g_ota_check_running = false;

  vTaskDelete(NULL);
}

void ota_check_async() {
  // Task liviana, no bloquea UI
  xTaskCreatePinnedToCore(ota_check_task, "ota_check", 8192, NULL, 1, NULL, 0);
}

// ---------- OTA START TASK ----------
static void ota_start_task(void* pv) {
  (void)pv;

  if (!ota_network_ready()) {
    set_status("Sin WiFi");
    g_ota_active = false;
    vTaskDelete(NULL);
    return;
  }

  if (!g_ota_available || strlen(g_ota_bin_url) == 0) {
    set_status("No hay update");
    g_ota_active = false;
    vTaskDelete(NULL);
    return;
  }

  s_cancel_req = false;
  g_ota_progress = 0;
  g_ota_active = true;
  set_status("Descargando...");

  if (s_cancel_req) {
    set_status("Cancelado");
    g_ota_active = false;
    vTaskDelete(NULL);
    return;
  }

  esp_http_client_config_t http_cfg = {};
  http_cfg.url = g_ota_bin_url;
  http_cfg.timeout_ms = (int)OTA_HTTP_TIMEOUT_MS;
  http_cfg.transport_type = HTTP_TRANSPORT_OVER_SSL;
  http_cfg.crt_bundle_attach = esp_crt_bundle_attach;

  esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
  if (!client) {
    set_status("HTTP init fail");
    g_ota_active = false;
    vTaskDelete(NULL);
    return;
  }

  ota_apply_auth_headers(client);
  esp_err_t err;
  int content_length = 0;
  int http_status = 0;
  int redirect_count = 0;

  while (true) {
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_http_client_open failed: %s", esp_err_to_name(err));
      esp_http_client_cleanup(client);
      set_status("HTTP open fail");
      g_ota_active = false;
      vTaskDelete(NULL);
      return;
    }

    content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) content_length = 0;
    http_status = esp_http_client_get_status_code(client);

    if (http_status == 301 || http_status == 302 || http_status == 303 || http_status == 307 || http_status == 308) {
      if (redirect_count >= 5) {
        ESP_LOGE(TAG, "Too many redirects while OTA download");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        set_status("HTTP redirect fail");
        g_ota_active = false;
        vTaskDelete(NULL);
        return;
      }

      ESP_LOGW(TAG, "OTA redirect HTTP %d (step %d)", http_status, redirect_count + 1);
      set_status("Siguiendo servidor...");
      esp_http_client_set_redirection(client);
      esp_http_client_close(client);
      ota_apply_auth_headers(client);
      redirect_count++;
      continue;
    }

    if (http_status != 200) {
      ESP_LOGE(TAG, "OTA HTTP status not OK: %d", http_status);
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
      set_status("HTTP bin error");
      g_ota_active = false;
      vTaskDelete(NULL);
      return;
    }

    break;
  }

  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
  if (!update_partition) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    set_status("Particion OTA invalida");
    g_ota_active = false;
    vTaskDelete(NULL);
    return;
  }

  esp_ota_handle_t ota_handle = 0;
  err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    set_status("Update.begin fail");
    g_ota_active = false;
    vTaskDelete(NULL);
    return;
  }

  set_status("Cargando...");

  uint8_t buffer[1024];
  int total_read = 0;
  bool failed = false;
  const char *fail_status = "OTA fallo";
  int64_t last_progress_us = esp_timer_get_time();

  while (true) {
    if (s_cancel_req) {
      set_status("Cancelado");
      failed = true;
      fail_status = "Cancelado";
      break;
    }

    int read_len = esp_http_client_read(client, (char *)buffer, sizeof(buffer));
    if (read_len == -ESP_ERR_HTTP_EAGAIN) {
      int64_t stall_ms = (esp_timer_get_time() - last_progress_us) / 1000;
      if (stall_ms > OTA_HTTP_STALL_TIMEOUT_MS) {
        ESP_LOGE(TAG, "OTA stalled for %lld ms", stall_ms);
        set_status("Descarga timeout");
        fail_status = "Descarga timeout";
        failed = true;
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    if (read_len < 0) {
      ESP_LOGE(TAG, "esp_http_client_read failed: %d", read_len);
      set_status("HTTP read fail");
      fail_status = "HTTP read fail";
      failed = true;
      break;
    }

    if (read_len == 0) {
      if (!esp_http_client_is_complete_data_received(client)) {
        int64_t stall_ms = (esp_timer_get_time() - last_progress_us) / 1000;
        if (stall_ms > OTA_HTTP_STALL_TIMEOUT_MS) {
          ESP_LOGE(TAG, "OTA incomplete/stalled for %lld ms", stall_ms);
          set_status("Descarga timeout");
          fail_status = "Descarga timeout";
          failed = true;
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
        continue;
      }
      break;
    }

    err = esp_ota_write(ota_handle, buffer, (size_t)read_len);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
      set_status("OTA write fail");
      fail_status = "OTA write fail";
      failed = true;
      break;
    }

    total_read += read_len;
    last_progress_us = esp_timer_get_time();
    if (content_length > 0) {
      int p = (total_read * 100) / content_length;
      if (p < 0) p = 0;
      if (p > 100) p = 100;
      g_ota_progress = p;
    }
  }

  if (!failed) {
    err = esp_ota_end(ota_handle);
    if (err == ESP_OK) {
      err = esp_ota_set_boot_partition(update_partition);
    }
    if (err != ESP_OK) {
      failed = true;
    }
  } else {
    esp_ota_abort(ota_handle);
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  if (failed) {
    set_status(fail_status);
    g_ota_active = false;
    vTaskDelete(NULL);
    return;
  }

  g_ota_progress = 100;
  set_status("OK, reiniciando");

  vTaskDelay(pdMS_TO_TICKS(600));
  esp_restart();
}

void ota_start_async() {
  if (g_ota_active) return;
  xTaskCreatePinnedToCore(ota_start_task, "ota_start", 12288, NULL, 2, NULL, 0);
}
