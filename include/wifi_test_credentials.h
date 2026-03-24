#pragma once

// Temporary WiFi credentials for bench/testing deployments.
// Fill these values for quick auto-connect without UI provisioning.
// WARNING: Avoid committing real credentials to shared repositories.

#ifndef WIFI_TEST_SSID
#define WIFI_TEST_SSID "JYMPCB"
#endif

#ifndef WIFI_TEST_PASS
#define WIFI_TEST_PASS "JYMPCB2023"
#endif

// 0: use saved STA credentials when available
// 1: always override saved STA credentials with WIFI_TEST_* values
#ifndef WIFI_TEST_OVERRIDE_SAVED
#define WIFI_TEST_OVERRIDE_SAVED 1
#endif
