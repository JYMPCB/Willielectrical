# OTA online con GitHub (público ahora, privado después)

## 1) Requisitos actuales del firmware

El firmware consulta un manifest JSON en:

- `OTA_MANIFEST_URL` (hoy: `https://jympcb.github.io/Willielectrical/ota/latest.json`)

Formato esperado del manifest:

```json
{
  "version": "0.1.1",
  "bin_url": "https://github.com/jympcb/Willielectrical/releases/download/v0.1.1/will.bin",
  "notes": "Cambios de la versión"
}
```

Regla clave: solo actualiza si `version` del manifest es mayor que `g_fw_version` (semver `MAJOR.MINOR.PATCH`).

---

## 2) Configurar GitHub para OTA público (prueba ahora)

### A. GitHub Pages (para el manifest)

1. En GitHub: **Settings > Pages**.
2. Source: **Deploy from a branch**.
3. Branch: `main` y carpeta `/ (root)`.
4. Guardar.
5. Confirmar que abre: `https://jympcb.github.io/Willielectrical/ota/latest.json`.

### B. Release (para el binario)

1. Crear tag/release: `v0.1.1`.
2. Subir asset `will.bin` al release.
3. Verificar URL del binario:
   - `https://github.com/jympcb/Willielectrical/releases/download/v0.1.1/will.bin`

---

## 3) Flujo recomendado por versión

1. Subir `FW_VERSION` en código (ej. `0.1.1`).
2. `build` para generar `build/Will.bin`.
3. Renombrar/copiar a `will.bin` para release.
4. Publicar release `v0.1.1` con `will.bin`.
5. Actualizar `ota/latest.json` con:
   - `version: "0.1.1"`
   - `bin_url` apuntando a release `v0.1.1`
6. `git push` del manifest.
7. En el equipo con firmware anterior, botón **Check update** y luego **Actualizar**.

---

## 4) Importante para la primera prueba OTA

Si el equipo ya corre `0.1.0`, para que vea update el manifest debe decir `0.1.1` o mayor.

---

## 5) Pasar a repo privado (recomendación segura)

Para OTA en dispositivos, **no es seguro** guardar un token de GitHub en firmware.

### Opción recomendada (segura)

- Mantener repo/releases privados.
- Usar un endpoint backend/proxy propio (Cloudflare Worker, Vercel, API propia) que:
  - valida cliente/dispositivo
  - usa token en servidor (no en firmware)
  - entrega manifest/bin o URL temporal firmada

El firmware solo habla con ese endpoint público controlado por vos.

### Opción posible pero no recomendada

El firmware ahora soporta headers opcionales (`OTA_AUTH_HEADER_NAME` / `OTA_AUTH_HEADER_VALUE`) para autenticación HTTP.
Funciona técnicamente, pero el secreto queda extraíble del binario.

---

## 6) Troubleshooting rápido

- `Manifest HTTP error`: revisar URL de Pages y estado público del JSON.
- `Al dia`: versión del manifest no es mayor a la del equipo.
- `HTTP open fail` al descargar bin: URL de release inválida o asset no publicado.
- OTA no arranca: verificar Wi-Fi conectado e IP válida.
