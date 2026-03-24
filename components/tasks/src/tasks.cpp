
#include "tasks.h"

#include "esp_timer.h"
#include "esp_log.h"
#include <math.h>
#include <time.h>
#include <lvgl.h>

#include "app_globals.h"
#include "app_state.h"
#include "training_interval.h"
#include "display_port.h"
#include "rgb_gpio.h"
#include "vfd_link.h"

static const char* TAG = "tasks";

static inline uint32_t now_ms()
{
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static float clampf_local(float v, float lo, float hi)
{
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static float pick_effective_speed_kmh(const vfd_status_t *st)
{
  if (!st) {
    return 0.0f;
  }

  float actual_kmh = clampf_local(((float)st->actual_speed_kmh_x100) / 100.0f, 0.0f, 25.0f);
  float target_kmh = clampf_local(((float)st->target_speed_kmh_x100) / 100.0f, 0.0f, 25.0f);

  bool running_hint = (st->run_fb != 0) ||
                      (st->run_cmd != 0) ||
                      (st->state == VFD_STATE_RUNNING);

  if (actual_kmh >= 0.2f) {
    return actual_kmh;
  }

  if (running_hint && target_kmh >= 0.2f) {
    return target_kmh;
  }

  return 0.0f;
}

// Approximation for treadmill kcal estimation based on speed bands.
static float kcal_per_kg_per_km(float speed_kmh)
{
  if (speed_kmh < 4.0f) return 0.70f;
  if (speed_kmh < 6.0f) return 0.80f;
  if (speed_kmh < 8.0f) return 0.90f;
  if (speed_kmh < 10.0f) return 0.98f;
  return 1.05f;
}

void mathTask(void *pv)
{
  (void)pv;

  const TickType_t period = pdMS_TO_TICKS(200);
  TickType_t last_wake = xTaskGetTickCount();
  uint32_t last_ms = now_ms();

  uint32_t elapsed_acc_ms = 0;
  float calories_frac = 0.0f;
  train_state_t prev_state = g_app.train_state;

  for(;;) {
    vTaskDelayUntil(&last_wake, period);

    uint32_t now = now_ms();
    uint32_t dt_ms = now - last_ms;
    last_ms = now;

    if (dt_ms > 2000U) {
      dt_ms = 200U;
    }

    if (g_reset_req) {
      g_reset_req = false;
      g_app.elapsed_time_s = 0;
      g_app.distance_km = 0.0f;
      g_app.calories = 0;
      g_app.current_speed_kmh = 0.0f;
      elapsed_acc_ms = 0;
      calories_frac = 0.0f;
      g_kcal_total_live = 0.0f;
      g_app.ui_req_running_refresh = true;
      continue;
    }

    if (g_app.train_state != prev_state) {
      if (g_app.train_state == TRAIN_STATE_RUNNING) {
        elapsed_acc_ms = 0;
        calories_frac = 0.0f;
      }

      if (g_app.train_state == TRAIN_STATE_IDLE) {
        g_app.current_speed_kmh = 0.0f;
      }

      prev_state = g_app.train_state;
      g_app.ui_req_running_refresh = true;
    }

    vfd_status_t st;
    bool have_status = vfd_link_get_status(&st);

    if (g_app.train_state == TRAIN_STATE_RUNNING || g_app.train_state == TRAIN_STATE_STARTING) {
      if (have_status) {
        g_app.current_speed_kmh = pick_effective_speed_kmh(&st);
      } else {
        // Ultimate fallback to avoid freezing metrics if status is temporarily unavailable.
        g_app.current_speed_kmh = clampf_local(g_app.target_speed_kmh, 0.0f, 25.0f);
      }
    }

    if (g_app.train_state == TRAIN_STATE_RUNNING) {
      float speed_kmh = g_app.current_speed_kmh;
      float delta_dist_km = speed_kmh * ((float)dt_ms / 3600000.0f);
      if (delta_dist_km < 0.0f) {
        delta_dist_km = 0.0f;
      }

      g_app.distance_km += delta_dist_km;

      elapsed_acc_ms += dt_ms;
      while (elapsed_acc_ms >= 1000U) {
        elapsed_acc_ms -= 1000U;
        g_app.elapsed_time_s++;
      }

      float kcal_coeff = kcal_per_kg_per_km(speed_kmh);
      calories_frac += delta_dist_km * peso * kcal_coeff;

      if (calories_frac >= 1.0f) {
        uint32_t add = (uint32_t)calories_frac;
        g_app.calories += add;
        calories_frac -= (float)add;
      }

      g_kcal_total_live = (float)g_app.calories + calories_frac;
      g_app.ui_req_running_refresh = true;
    } else {
      elapsed_acc_ms = 0;
      calories_frac = 0.0f;
      g_kcal_total_live = (float)g_app.calories;
    }
  }
}

/*
void mathTask(void *pv)
{
 
  extern volatile bool g_workout_frozen;

  static bool startedOnce = false;

  const uint32_t tiempoVelMin = 2; // segundos sin giro para considerar stop

  uint32_t last_giro = 0;
  uint32_t counterNoGiro = 0;

  uint32_t last_ms = now_ms();
  uint32_t acc_ms  = 0;

  for(;;) {

    if(g_workout_frozen) {
    vTaskDelay(pdMS_TO_TICKS(50));  // o 100ms
    continue;                       // NO recalcular, pero la task sigue viva
    }    

    //vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
    // reset pedido desde UI
    if(g_reset_req) {
      g_reset_req = false;

      float espacio_now = espacio;
      sessionActive = false;
      startedOnce = false;

      tiempoHour = tiempoMin = tiempoSec = 0;
      espacio = 0.0f;

      acc_ms = 0;
      last_ms = now_ms();

      portENTER_CRITICAL(&g_isr_mux);
      g_giro = 0;
      g_period_cycles = 0;
      g_last_cycles = esp_cpu_get_cycle_count();
      portEXIT_CRITICAL(&g_isr_mux);

      g_speed_reset_req = true;

      // ocultar cartel pausa
      xSemaphoreTake(g_ui_mutex, portMAX_DELAY);
      g_ui.pausaVisible = false;
      xSemaphoreGive(g_ui_mutex);

      // mantener última config: volver a ARMED
      if(g_train_mode == TRAIN_INTERVAL) ti_rearm(espacio_now);
      else                              ti_stop();
    }

    ui_state_t out;
    xSemaphoreTake(g_ui_mutex, portMAX_DELAY);
    out = g_ui;                 // ✅ partir del estado actual (incluye speed/pace/cursor)
    xSemaphoreGive(g_ui_mutex);

    uint32_t giro_now;
    portENTER_CRITICAL(&g_isr_mux);
    giro_now = g_giro;
    portEXIT_CRITICAL(&g_isr_mux);

    bool hubo_giro = (giro_now != last_giro);
    if(hubo_giro) {
      counterNoGiro = 0;
      last_giro = giro_now;
    } else {
      counterNoGiro++;
    }

    // inicio inteligente
    if(!g_workout_frozen && !startedOnce && giro_now >= 3) {
      startedOnce = true;
      sessionActive = true;

      if(g_train_mode == TRAIN_INTERVAL) {
        ti_on_session_started(espacio);
      }
    }

    bool movimiento = (counterNoGiro <= tiempoVelMin);

    // Si terminó y está congelado, barras visibles y controles accesibles
    bool force_controls = g_workout_frozen; 
      
    out.headerHidden   = movimiento && !force_controls;
    out.settingsHidden = movimiento && !force_controls;
    out.showNetTime    = (!movimiento) || force_controls;
    // Mostrar reinicio si hay sesión activa O si quedó congelado el final
    out.showReinitBtn  = sessionActive || force_controls;

    // ---- tiempo ----
    uint32_t current_ms = now_ms();
    uint32_t dt_ms  = current_ms - last_ms;
    last_ms = current_ms;

    if(sessionActive && movimiento) {
      acc_ms += dt_ms;
      while(acc_ms >= 1000) {
        acc_ms -= 1000;
        tiempoSec++;
        if(tiempoSec >= 60) { tiempoSec = 0; tiempoMin++; }
        if(tiempoMin >= 60) { tiempoMin = 0; tiempoHour++; }
      }
    }

    if(g_train_mode != TRAIN_INTERVAL || g_interval_is_dist) {
      snprintf(out.trainTimeStr, sizeof(out.trainTimeStr),
              "%02u:%02u:%02u",
              (unsigned)tiempoHour,
              (unsigned)tiempoMin,
              (unsigned)tiempoSec);
    }
    
    //---rest api ------
    uint32_t elapsed = (uint32_t)tiempoHour * 3600u + (uint32_t)tiempoMin  * 60u + (uint32_t)tiempoSec;
    out.elapsed_s = elapsed; //---valores hacia rest api ------

    // ---- distancia ----
    float espacio_m = drdisco * giro_now;
    
    // ✅ esto es CLAVE para intervalado (usa la global)
    espacio = espacio_m;

    out.dist_m = espacio_m; //---valores hacia rest api ------

    if(espacio_m < 1000) {
      snprintf(out.distStr, sizeof(out.distStr), "%.0f", espacio_m);
      strcpy(out.distUnit, "mt");
    } else {
      snprintf(out.distStr, sizeof(out.distStr), "%.3f", espacio_m * 0.001f);
      strcpy(out.distUnit, "km");
    }

    // ---- calorías ----
    float v_now;
    xSemaphoreTake(g_ui_mutex, portMAX_DELAY);
    v_now = g_ui.velocidad;
    xSemaphoreGive(g_ui_mutex);

    float factor = 0.65f + 0.015f * logf(v_now + 1.0f);
    float calorias = peso * (espacio_m * 0.001f) * factor;
    snprintf(out.calStr, sizeof(out.calStr), "%.1f", calorias);
    g_kcal_total_live = calorias;    
    out.kcal = calorias; //---valores hacia rest api ------
    // ---- hora net ----
    if(out.showNetTime && habLocalTime) {
      time_t now = time(nullptr);
      if(now > 1700000000) {
        struct tm dt;
        localtime_r(&now, &dt);
        strftime(out.timeStr, sizeof(out.timeStr), "%H:%M", &dt);
        strftime(out.dateStr, sizeof(out.dateStr), "%d/%m/%Y", &dt);
      }
    }

    // Intervalado: update 1s
    if(g_train_mode == TRAIN_INTERVAL) {
      ti_update(1000, espacio);
    }

    // ✅ mezclar overlay del intervalado sobre OUT (pausa + restante por tiempo)
    ti_apply_overlay_to_ui(&out);

    out.session_active = sessionActive;     //---valores hacia rest api ------
    out.workout_frozen = g_workout_frozen;  //---valores hacia rest api ------

    // publicar snapshot
    xSemaphoreTake(g_ui_mutex, portMAX_DELAY);
    g_ui = out;
    xSemaphoreGive(g_ui_mutex);
  }
}
*/

//TAREAS PARA PRUEBAS
volatile uint32_t g_gui_worstGap_ms = 0;
volatile uint32_t g_gui_last_lv_us  = 0;
volatile uint32_t g_gui_stack_hw    = 0;
void logTask(void *){
  for(;;){
    ESP_LOGI(TAG, "[gui] worstGap=%lu ms lv=%lu us stackHW=%lu words",
      (unsigned long)g_gui_worstGap_ms,
      (unsigned long)g_gui_last_lv_us,
      (unsigned long)g_gui_stack_hw
    );
    g_gui_worstGap_ms = 0;

    /*Serial.printf("[flush] worst=%lu us last=%lu us\n",
    (unsigned long)g_flush_worst_us, (unsigned long)g_flush_last_us);
    g_flush_worst_us = 0;*/

    ESP_LOGI(TAG, "[flush] count/s=%lu worst=%lu us worstArea=%ldx%ld",
    (unsigned long)g_flush_count,
    (unsigned long)g_flush_worst_us,
    (long)g_flush_worst_w, (long)g_flush_worst_h
    );
    g_flush_count = 0;
    g_flush_worst_us = 0;

    ESP_LOGI(TAG, "[touch] count/s=%lu worst=%lu us last=%lu us",
    (unsigned long)g_touch_count,
    (unsigned long)g_touch_worst_us,
    (unsigned long)g_touch_last_us
    );
    g_touch_count = 0;
    g_touch_worst_us = 0;

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/*void speedTask(void *pv)
{
  extern volatile bool g_workout_frozen;

  const TickType_t period = pdMS_TO_TICKS(100);
  TickType_t last = xTaskGetTickCount();

  const float v_max = 40.0f;

  // ---- umbral físico: período a 40 km/h según drdisco (m) ----
  // drdisco = 0.80384 m -> t40_us ≈ 72345 us
  const uint32_t t40_us = (uint32_t)((drdisco / (v_max / 3.6f)) * 1e6f + 0.5f);

  // Margen duro: rechaza pulsos más rápidos que ~40km/h (con tolerancia)
  const float margin = 0.9f;                 // 0.85..0.90 (más alto = más duro)
  const uint32_t t_min_us = (uint32_t)(t40_us * margin);

  // ---- filtro duro de picos ----
  static float v_hold = 0.0f;                 // último valor creíble
  static uint8_t spike_cnt = 0;
  const float jump_allow = 5.0f;              // km/h por tick (100ms)
  const uint8_t need_confirm = 2;             // lecturas seguidas para aceptar salto grande

  // opcional: si falta un pulso, mantenemos el último período válido por 1 tick
  static uint32_t per_us_last_valid = 0;

  for(;;) {

    if(g_workout_frozen) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    vTaskDelayUntil(&last, period);

    // reset pedido desde mathTask
    if(g_speed_reset_req) {
      g_speed_reset_req = false;
      v_hold = 0.0f;
      spike_cnt = 0;
      per_us_last_valid = 0;
      
      xSemaphoreTake(g_ui_mutex, portMAX_DELAY);
      g_ui.velocidad = 0;
      g_ui.cursorAngle = 0;
      strcpy(g_ui.speedStr, "0.0");
      strcpy(g_ui.ritmoStr, "00'00\"");
      xSemaphoreGive(g_ui_mutex);
      continue;
    }

    uint32_t per_cycles;
    portENTER_CRITICAL(&g_isr_mux);
    per_cycles = g_period_cycles;
    portEXIT_CRITICAL(&g_isr_mux);

    uint32_t per_us = (per_cycles > 0) ? (per_cycles / g_cpu_mhz) : 0;

    //bool new_sample = (per_cycles > 0);
    // ---- filtro físico: descartar pulsos demasiado rápidos (glitch/rebote) ----
    if(per_us > 0 && per_us < t_min_us) {
      per_us = 0;
    }

    // opcional: si viene 0 un tick, usar el último válido para evitar jitter
    if(per_us == 0 && per_us_last_valid != 0) {
      per_us = per_us_last_valid;
    }

    float velocidad = 0.0f;
    if(per_us > 0) {
      per_us_last_valid = per_us;

      float t = per_us * 1e-6f;
      velocidad = (drdisco / t) * 3.6f;

      if(velocidad < 0.1f) velocidad = 0.0f;
      if(velocidad > v_max) velocidad = v_max;
    }

    // ---- confirmación de picos (duro) ----
    if(!sessionActive) {
      velocidad = 0.0f;
      v_hold = 0.0f;
      spike_cnt = 0;
    } else {      
      if(velocidad > (v_hold + jump_allow)) {
        spike_cnt++;
        if(spike_cnt < need_confirm) {
          velocidad = v_hold;   // ignorar pico aislado
        } else {
          v_hold = velocidad;   // aceptado si se sostuvo
          spike_cnt = 0;
        }
      } else {
        spike_cnt = 0;
        v_hold = velocidad;
      }
      
    }

    char speedStr[16];
    snprintf(speedStr, sizeof(speedStr), "%.1f", (double)velocidad);

    char ritmoStr[16];
    uint8_t r = 0, g = 0, b = 0;

    if(velocidad > 0.1f) {
      float ritmo = 60.0f / velocidad;
      if(ritmo > 30) ritmo = 30;

      pace_to_rgb(ritmo, r, g, b);

      int rm = (int)ritmo;
      int rs = (int)((ritmo - rm) * 60.0f + 0.5f);
      if(rs >= 60) { rs = 0; rm++; }

      snprintf(ritmoStr, sizeof(ritmoStr), "%02d'%02d\"", rm, rs);

      if(sessionActive) {
        rgb_set_target_u8(r, g, b);
      } else {
        r = g = b = 0;
      }
    } else {
      strcpy(ritmoStr, "00'00\"");
      r = g = b = 0;
    }    

    int angle = (int)((sessionActive ? v_hold : 0.0f) * 45.0f);
    if(angle < 0) angle = 0;
    if(angle > (int)(v_max * 45.0f)) angle = (int)(v_max * 45.0f);

    xSemaphoreTake(g_ui_mutex, portMAX_DELAY);
    g_ui.velocidad = velocidad;
    g_ui.cursorAngle = angle;
    strlcpy(g_ui.speedStr, speedStr, sizeof(g_ui.speedStr));
    strlcpy(g_ui.ritmoStr, ritmoStr, sizeof(g_ui.ritmoStr));
    xSemaphoreGive(g_ui_mutex);
  }
}*/

void guiTask(void *pv)
{
  uint32_t last_tick_ms = now_ms();

  for(;;){
    uint32_t now_tick_ms = now_ms();
    uint32_t elapsed_ms = now_tick_ms - last_tick_ms;
    last_tick_ms = now_tick_ms;
    if (elapsed_ms == 0) elapsed_ms = 1;

    g_gui_stack_hw = (uint32_t)uxTaskGetStackHighWaterMark(NULL);

    lv_tick_inc(elapsed_ms);

    int64_t lv_start_us = esp_timer_get_time();
    uint32_t wait_ms = display_port_lvgl_handler();
    g_gui_last_lv_us = (uint32_t)(esp_timer_get_time() - lv_start_us);

    // Reducir frecuencia de refresh en escenarios de alta carga de bus (WiFi scan/OTA/config)
    // para evitar solapamiento en MIPI-DPI: "previous draw operation is not finished".
    bool high_bus_load_ui = habConfig || g_ota_check_running || g_ota_active;

    if (high_bus_load_ui) {
      if (wait_ms < 30) wait_ms = 30;  // ~33 Hz mientras hay carga alta
      if (wait_ms > 50) wait_ms = 50;
    } else {
      // Clamp normal
      if(wait_ms > 16) wait_ms = 16;   // ~60 Hz max en operación normal
      if(wait_ms < 2)  wait_ms = 2;
    }

    vTaskDelay(pdMS_TO_TICKS(wait_ms));
  }
}

/*void guiTask(void *pv)
{
    ESP_LOGI(TAG, "guiTask started");

    uint32_t last_tick_ms = now_ms();

    for(;;) {
        uint32_t now_tick_ms = now_ms();
        uint32_t elapsed_ms = now_tick_ms - last_tick_ms;
        last_tick_ms = now_tick_ms;
        if (elapsed_ms == 0) elapsed_ms = 1;

        g_gui_stack_hw = (uint32_t)uxTaskGetStackHighWaterMark(NULL);

        //ESP_LOGI(TAG, "before lv_tick_inc");
        lv_tick_inc(elapsed_ms);

        //ESP_LOGI(TAG, "before lv_timer_handler");
        uint32_t wait_ms = display_port_lvgl_handler();

        //ESP_LOGI(TAG, "after lv_timer_handler, wait_ms=%lu", (unsigned long)wait_ms);

        if(wait_ms > 16) wait_ms = 16;
        if(wait_ms < 2)  wait_ms = 2;

        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}*/

void startTasks()
{
    BaseType_t rc = xTaskCreate(guiTask, "gui", 12288, NULL, 4, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "Failed to create gui task");
    } else {
        ESP_LOGI(TAG, "gui task created");
    }
    rc = xTaskCreatePinnedToCore(mathTask, "math", 4096, NULL, 3, &g_mathTaskHandle, 0);
    if (rc != pdPASS) {
      ESP_LOGE(TAG, "Failed to create math task");
    } else {
      ESP_LOGI(TAG, "math task created");
    }
  //xTaskCreatePinnedToCore(speedTask, "speed", 4096, NULL, 3, &g_speedTaskHandle, 0);
}

