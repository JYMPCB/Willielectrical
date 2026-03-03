#pragma once


// URL del manifest en GitHub Pages
#ifndef OTA_MANIFEST_URL
#define OTA_MANIFEST_URL "https://jympcb.github.io/Willielectrical/ota/latest.json"
#endif

// Headers opcionales para OTA en entornos privados/proxy
// Ejemplo:
//   -DOTA_AUTH_HEADER_NAME="\"Authorization\""
//   -DOTA_AUTH_HEADER_VALUE="\"Bearer <token>\""
#ifndef OTA_AUTH_HEADER_NAME
#define OTA_AUTH_HEADER_NAME ""
#endif

#ifndef OTA_AUTH_HEADER_VALUE
#define OTA_AUTH_HEADER_VALUE ""
#endif

// Llamalo al conectar WiFi y/o cada X minutos
void ota_check_async();

// OTA local via webserver (subida de .bin desde navegador)
void ota_local_start();
void ota_local_stop();

// Lo llamás cuando el usuario toca "Actualizar"
void ota_start_async();

// Opcional: para que UI pueda “cancelar” antes de flashear (simple)
void ota_request_cancel();
