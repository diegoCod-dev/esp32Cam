# 📷 ESP32 SmartCam

> Sistema modular de captura, almacenamiento y consulta de imágenes desarrollado sobre una placa **ESP32-CAM** utilizando **ESP-IDF**.

El proyecto permite capturar fotografías, almacenarlas en una tarjeta microSD y acceder a ellas mediante una interfaz web generada por el propio ESP32. También incorpora comunicación **ESP-NOW**, lo que permite que otro dispositivo ESP32 solicite capturas de forma inalámbrica, incluso sin depender de un router o de una conexión a Internet.

---

## ✨ Funcionalidades

- Captura de imágenes mediante ESP32-CAM con asignación obligatoria en **PSRAM**.
- Almacenamiento de fotografías en tarjeta microSD mediante exclusión mutua (*mutex*).
- Access Point Wi-Fi propio y conexión a red externa (Modo híbrido APSTA).
- Servidor web HTTP con soporte de envío por bloques (*chunks*) para acceder a las fotografías.
- Comunicación directa entre dispositivos mediante ESP-NOW.
- Recepción remota del comando `CAPTURAR`.
- Confirmación de captura mediante los mensajes `FOTO_OK` y `OK`.
- Captura automática de respaldo mediante temporizador independiente.
- **Configuración completa mediante Kconfig** tanto para los parámetros Wi-Fi, pines de hardware (`esp32Cam`) como para la habilitación de la memoria PSRAM.
- Arquitectura modular basada en componentes de ESP-IDF documentados con **Doxygen**.

---

## 🧰 Requisitos de Hardware y PSRAM

- **Placa ESP32-CAM** (Modelo estándar AI-Thinker).
- **Cámara OV2640** integrada.
- **Memoria PSRAM obligatoria**: Es fundamental que el módulo cuente con PSRAM física (por ejemplo, el chip ESP32-S con la RAM externa de 4MB o 8MB añadida). Sin la PSRAM activa, la cámara no dispondrá de suficiente búfer de video y el sistema fallará al iniciar.
- Tarjeta microSD formateada (FAT/FAT32).
- Fuente de alimentación estable (capaz de soportar los picos de corriente del Wi-Fi y la cámara).
- ESP-32S NodeMCU como dispositivo remoto para pruebas mediante ESP-NOW.

---

## 💻 Software

El proyecto está desarrollado utilizando:

- Visual Studio Code.
- ESP-IDF (v6.0.3 o superior).
- Extensión ESP-IDF para Visual Studio Code.
- FreeRTOS.
- Git y GitHub.

---

## 🧱 Arquitectura del proyecto

El sistema está organizado en componentes independientes para separar responsabilidades y facilitar su mantenimiento y reutilización.

```text
components/
├── esp32Cam/
├── sd/
├── wifi_server/
├── espnow_manager/
├── capture_timer/
└── mqtt_manager/
```

### Componentes principales

#### `esp32Cam`
Responsable de la inicialización y utilización de la cámara OV2640. Fuerza la asignación de búferes en **PSRAM** (`CAMERA_FB_IN_PSRAM`) para evitar desbordamientos y es totalmente configurable mediante **Kconfig**.

#### `sd`
Gestiona la tarjeta microSD, el montaje del sistema de archivos FAT y el almacenamiento seguro de las imágenes utilizando un *mutex* para entornos multihilo.

#### `wifi_server`
Unifica la gestión de red en modo híbrido **APSTA** (Access Point + Station) junto al servidor HTTP web que permite visualizar y descargar las imágenes de la tarjeta SD. Sus credenciales y parámetros de red se configuran a través de **Kconfig**.

#### `espnow_manager`
Componente genérico destinado a la comunicación ESP-NOW. Permite inicializar el servicio, registrar *peers*, enviar información y recibir datos mediante callbacks desvinculados de la lógica de negocio.

#### `capture_timer`
Temporizador independiente basado en FreeRTOS utilizado para generar capturas automáticas de respaldo cuando no se reciben solicitudes externas durante un período determinado.

#### `mqtt_manager`
Componente preparado para la publicación de telemetría y estados de la cámara hacia un broker MQTT local (por ejemplo, en Docker).

---

## 🔄 Flujo de funcionamiento

La ESP32-CAM puede generar una fotografía de dos maneras: mediante una solicitud ESP-NOW o por vencimiento del temporizador de respaldo.

### Captura solicitada mediante ESP-NOW

Un dispositivo ESP32 remoto solicita una fotografía enviando el comando:

```text
CAPTURAR
```

La secuencia completa es:

```text
ESP-32S NodeMCU                    ESP32-CAM
       │                               │
       │──────── CAPTURAR ────────────>│
       │                               │
       │                         Captura imagen (vía PSRAM)
       │                         Guarda en microSD
       │                               │
       │<──────── FOTO_OK ─────────────│
       │                               │
       │─────────── OK ───────────────>│
```

En la implementación actual, el ESP-32S NodeMCU envía `CAPTURAR` al presionar su botón. La ESP32-CAM realiza la captura, guarda la imagen, reporta el estado vía MQTT, notifica al NodeMCU con `FOTO_OK`, y este confirma la recepción mediante `OK`.

### Captura automática de respaldo

Si la ESP32-CAM no recibe el comando `CAPTURAR` durante el intervalo configurado, el componente `capture_timer` genera automáticamente una nueva solicitud de captura.

El intervalo de respaldo configurado es de:

```text
40 segundos
```

Cada vez que se procesa una solicitud externa o una captura automática, el temporizador se reinicia.

---

## ⚙️ Configuración del Sistema (Kconfig y PSRAM)

Para garantizar la portabilidad y el correcto funcionamiento de la cámara, es necesario validar que la **PSRAM esté habilitada** en el menú de configuración de ESP-IDF.

### Abrir el menú de configuración

Desde una terminal con el entorno de ESP-IDF activado:

```bash
idf.py menuconfig
```

O desde Visual Studio Code:
1. Abrir la paleta de comandos (`F1` o `Ctrl+Shift+P`).
2. Seleccionar: **ESP-IDF: SDK Configuration editor (menuconfig)**.

### Pasos críticos en el menú:
1. **Habilitar la PSRAM:** Navega a `Component config` ➔ `ESP32-specific` (o `PHY` / `SPIRAM`) y asegúrate de marcar:
   - `[*] Support for external, SPI-connected RAM` (SPI RAM support).
   - `SPI RAM access method` ➔ Seleccionar *Try to allocate RAM in SPIRAM first* o *Allocated separately*.
2. **Configuración de componentes:** En `Component config` podrás ajustar los pines de la cámara, las credenciales del Wi-Fi de la red local y del Access Point definido en los Kconfig de tus componentes.

> [!CAUTION]
> No se recomienda publicar credenciales reales en repositorios públicos. Asegúrate de excluir el archivo `sdkconfig` mediante `.gitignore`.

---

## 🔧 Compilación y carga

### Compilar el proyecto

```bash
idf.py build
```

### Grabar el firmware en la placa

```bash
idf.py flash
```

### Abrir el monitor serie

```bash
idf.py monitor
```

También puedes realizar estas acciones directamente desde los accesos rápidos de la extensión de ESP-IDF en Visual Studio Code.