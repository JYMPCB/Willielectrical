#include "service_mgr.h"
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"

static const char* NS = "service";

static bool nvs_read_u32(const char *key, uint32_t *out)
{
  if (!out) return false;
  nvs_handle_t h;
  if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) return false;
  esp_err_t err = nvs_get_u32(h, key, out);
  nvs_close(h);
  return err == ESP_OK;
}

static bool nvs_read_u8(const char *key, uint8_t *out)
{
  if (!out) return false;
  nvs_handle_t h;
  if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) return false;
  esp_err_t err = nvs_get_u8(h, key, out);
  nvs_close(h);
  return err == ESP_OK;
}

static void nvs_write_u32(const char *key, uint32_t value)
{
  nvs_handle_t h;
  if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return;
  if (nvs_set_u32(h, key, value) == ESP_OK) {
    nvs_commit(h);
  }
  nvs_close(h);
}

static void nvs_write_u8(const char *key, uint8_t value)
{
  nvs_handle_t h;
  if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return;
  if (nvs_set_u8(h, key, value) == ESP_OK) {
    nvs_commit(h);
  }
  nvs_close(h);
}

void ServiceMgr::begin() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    if (nvs_flash_erase() == ESP_OK) {
      nvs_flash_init();
    }
  }

  st.last_ack_epoch = 0;
  st.pending = false;
  st.postponed = false;

  (void)nvs_read_u32("last_ack", &st.last_ack_epoch);
  uint8_t v = 0;
  if (nvs_read_u8("pend", &v)) st.pending = (v != 0);
  if (nvs_read_u8("post", &v)) st.postponed = (v != 0);
}

void ServiceMgr::save() {
  nvs_write_u32("last_ack", st.last_ack_epoch);
  nvs_write_u8("pend", st.pending ? 1 : 0);
  nvs_write_u8("post", st.postponed ? 1 : 0);
}

int32_t ServiceMgr::days_left(time_t now_epoch) const {
  if(st.last_ack_epoch == 0) return (int32_t)SERVICE_DAYS;

  uint32_t now_u = (uint32_t)now_epoch;
  uint32_t last  = st.last_ack_epoch;

  uint32_t elapsed_sec = (now_u >= last) ? (now_u - last) : 0;
  int32_t elapsed_days = (int32_t)(elapsed_sec / 86400UL);

  return (int32_t)SERVICE_DAYS - elapsed_days;
}

bool ServiceMgr::evaluate_on_boot(time_t now_epoch) {
  // Si el usuario lo postergó, sigue pendiente.
  if(st.postponed) {
    st.pending = true;
    save();
    return true;
  }

  // Si no hay last_ack todavía, arrancamos a contar desde ahora (primera vez con hora válida)
  if(st.last_ack_epoch == 0) {
    st.last_ack_epoch = (uint32_t)now_epoch;
    st.pending = false;
    st.postponed = false;
    save();
    return false;
  }

  uint32_t now_u = (uint32_t)now_epoch;
  uint32_t elapsed = (now_u >= st.last_ack_epoch) ? (now_u - st.last_ack_epoch) : 0;

  if(elapsed >= SERVICE_SEC) {
    st.pending = true;
    st.postponed = false; // IMPORTANTE: vencido ≠ postergado
    save();
    return true;
  }

  st.pending = false;
  st.postponed = false;
  save();
  return false;
}

void ServiceMgr::onPostpone() {
  st.pending = true;
  st.postponed = true;
  save();
}

void ServiceMgr::onAccept(time_t now_epoch) {
  st.last_ack_epoch = (uint32_t)now_epoch;
  st.pending = false;
  st.postponed = false;
  save();
}

void ServiceMgr::formatConfigLabel(char* out, size_t n, time_t now_epoch, bool time_ok) const {
  if(st.postponed) {
    strlcpy(out, "Service: postergado", n);
    return;
  }

  if(!time_ok) {
    // Para mantener tus “2 textos” sin inventar un tercero,
    // podés mostrar días como "--"
    strlcpy(out, "Service en: -- dias", n);
    return;
  }

  int32_t left = days_left(now_epoch);
  if(left < 0) left = 0;  // no mostrar negativos en config

  snprintf(out, n, "Service en: %ld dias", (long)left);
}

void ServiceMgr::formatPopupText(char* out, size_t n, time_t now_epoch) const {
  if(st.postponed) {
    // texto simple: el usuario ya sabe que postergó
    strlcpy(out, "Service pendiente (postergado).", n);
    return;
  }

  // vencido
  int32_t left = days_left(now_epoch);
  if(left > 0) left = 0;

  strlcpy(out, "Mantenimiento requerido.\nPresiona Aceptar o Postergar.", n);
}
