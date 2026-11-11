# 📷 ESP32 SmartCam

> Sistema modular de captura, almacenamiento y consulta de imágenes desarrollado sobre una placa **ESP32-CAM** utilizando **ESP-IDF**.

El proyecto permite capturar fotografías, almacenarlas en una tarjeta microSD y acceder a ellas mediante una interfaz web generada por el propio ESP32. También incorpora comunicación **ESP-NOW**, lo que permite que otro dispositivo ESP32 solicite capturas de forma inalámbrica, incluso sin depender de un router o de una conexión a Internet.

---

## ✨ Funcionalidades

- Captura de imágenes mediante ESP32-CAM.
- Almacenamiento de fotografías en tarjeta microSD.
- Access Point Wi-Fi propio.
- Conexión a una red Wi-Fi externa mediante modo Station.
- Servidor web para acceder a las fotografías almacenadas.
- Comunicación directa entre dispositivos mediante ESP-NOW.
- Recepción remota del comando `CAPTURAR`.
- Confirmación de captura mediante los mensajes `FOTO_OK` y `OK`.
- Captura automática de respaldo mediante temporizador.
- Configuración Wi-Fi mediante Kconfig.
- Arquitectura modular basada en componentes de ESP-IDF.

---

## 🧰 Hardware

- Placa ESP32-CAM.
- Cámara integrada compatible con ESP32-CAM.
- Tarjeta microSD.
- Fuente de alimentación adecuada para la placa.
- ESP-32S NodeMCU como dispositivo remoto para pruebas mediante ESP-NOW.

---

## 💻 Software

El proyecto está desarrollado utilizando:

- Visual Studio Code.
- ESP-IDF.
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
├── wifi_manager/
├── web_server/
├── espnow_manager/
├── capture_timer/
└── mqtt_manager/
```

### Componentes principales

#### `esp32Cam`

Responsable de la inicialización y utilización de la cámara.

#### `sd`

Gestiona la tarjeta microSD y el almacenamiento de las imágenes capturadas.

#### `wifi_manager`

Gestiona la inicialización y configuración de Wi-Fi.

Está diseñado para admitir los siguientes modos:

- Station (`STA`).
- Access Point (`AP`).
- Access Point + Station (`APSTA`).

Su configuración se realiza mediante **Kconfig**, evitando guardar parámetros específicos directamente en el código fuente.

#### `web_server`

Implementa el servidor HTTP utilizado para acceder desde otro dispositivo a las fotografías almacenadas en la tarjeta microSD.

#### `espnow_manager`

Componente genérico destinado a la comunicación ESP-NOW.

Permite:

- Inicializar ESP-NOW.
- Registrar dispositivos remotos o *peers*.
- Enviar información.
- Recibir información mediante callbacks.

El componente no contiene lógica específica de la aplicación. Los comandos y las respuestas se interpretan desde la aplicación principal, lo que permite reutilizar `espnow_manager` en otros proyectos.

#### `capture_timer`

Temporizador independiente utilizado para generar capturas automáticas cuando no se reciben solicitudes externas durante un período determinado.

#### `mqtt_manager`

Componente reservado para una futura integración mediante MQTT. Actualmente no forma parte del flujo principal del sistema.

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
       │                         Captura imagen
       │                         Guarda en microSD
       │                               │
       │<──────── FOTO_OK ─────────────│
       │                               │
       │─────────── OK ───────────────>│
```

En la implementación de prueba, el ESP-32S NodeMCU envía `CAPTURAR` únicamente cuando se presiona su botón. La ESP32-CAM realiza la captura, guarda la imagen y responde `FOTO_OK`. Finalmente, el NodeMCU confirma la recepción mediante `OK`.

ESP-NOW permite que ambos dispositivos se comuniquen directamente sin requerir un router ni acceso a Internet.

### Captura automática de respaldo

La ESP32-CAM también dispone de un mecanismo de respaldo.

Si no recibe el comando `CAPTURAR` durante el intervalo configurado, el componente `capture_timer` genera automáticamente una nueva solicitud de captura.

El intervalo utilizado actualmente es de:

```text
40 segundos
```

Cada vez que se recibe una solicitud externa o se realiza una captura automática, el temporizador vuelve a comenzar.

---

## 📡 Configuración Wi-Fi con Kconfig

Los parámetros de red no se almacenan directamente dentro del código fuente del componente.

El componente `wifi_manager` utiliza el sistema **Kconfig** de ESP-IDF. Esto permite configurar cada proyecto sin modificar los archivos `.c` o `.h`.

### Abrir la configuración

Desde una terminal de ESP-IDF:

```bash
idf.py menuconfig
```

También puede abrirse desde Visual Studio Code:

```text
ESP-IDF: SDK Configuration editor (menuconfig)
```

Luego ingresar en:

```text
Component config
└── WiFi Manager Configuration
```

Desde este menú se pueden definir:

- Modo de funcionamiento Wi-Fi.
- SSID del Access Point.
- Contraseña del Access Point.
- SSID de la red Station.
- Contraseña de la red Station.
- Canal Wi-Fi.
- Cantidad máxima de conexiones.
- Cantidad máxima de reintentos de conexión.

Esta configuración permite reutilizar `wifi_manager` en otros proyectos sin modificar su implementación.

> [!CAUTION]
> No se recomienda publicar credenciales reales. Si el archivo `sdkconfig` contiene datos sensibles, debe excluirse del repositorio mediante `.gitignore`.

---

## 📨 Protocolo ESP-NOW

El sistema utiliza actualmente los siguientes mensajes:

| Mensaje | Origen | Destino | Función |
|---|---|---|---|
| `CAPTURAR` | ESP-32S NodeMCU | ESP32-CAM | Solicita una fotografía |
| `FOTO_OK` | ESP32-CAM | ESP-32S NodeMCU | Informa que la fotografía fue realizada y guardada |
| `OK` | ESP-32S NodeMCU | ESP32-CAM | Confirma la recepción de `FOTO_OK` |

Las direcciones MAC se definen en la aplicación y no dentro del componente genérico `espnow_manager`.

---

## 🌐 Servidor web

La ESP32-CAM crea un servidor HTTP que permite acceder a las fotografías almacenadas en la tarjeta microSD.

El servidor web se mantiene separado de `wifi_manager`:

- `wifi_manager` administra la conectividad.
- `web_server` administra las rutas HTTP y el acceso a las imágenes.

Esta separación evita que la lógica de red quede acoplada al manejo de fotografías.

---

## 🔧 Compilación y carga

### Compilar

```bash
idf.py build
```

### Grabar el firmware

```bash
idf.py flash
```

### Abrir el monitor serie

```bash
idf.py monitor
```

También pueden utilizarse las acciones equivalentes de la extensión ESP-IDF para Visual Studio Code:

```text
Build → Flash → Monitor
```

---

## 📁 Diseño modular

La aplicación principal coordina los componentes, pero cada uno mantiene una responsabilidad específica:

```text
Aplicación principal
│
├── Cámara
├── microSD
├── Wi-Fi
├── Servidor web
├── ESP-NOW
├── Temporizador de respaldo
└── MQTT (integración futura)
```

Esta organización facilita:

- El mantenimiento del código.
- La prueba independiente de cada módulo.
- La reutilización de componentes.
- La incorporación de nuevas funcionalidades.
- La adaptación del proyecto a otras placas ESP32.
