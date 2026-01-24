# Базовый проект для JC1060P470C_I_W на PlatformIO

Минимальный шаблон для платы JC1060P470C_I_W (ESP32-P4 + LCD 1024x600) с Arduino Core и LVGL. Проект запускает LVGL, инициализирует дисплей JD9165 и тачскрин GT911, выводит приветственный экран с кнопкой и отладочные логи по UART0/USB-CDC.

## Требования
- PlatformIO (CLI или VS Code) с установленной платформой `https://github.com/pioarduino/platform-espressif32.git#53.03.13+github`.
- Плата JC1060P470C_I_W (используется профиль `esp32-p4-evboard`).
- Подключение USB для прошивки и UART0 для логов (115200 бод).

## Сборка и прошивка
1. Установите зависимости PlatformIO (автоматически подтянутся LVGL 9.4.0 и платформа Espressif32).
2. Собрать: `pio run`
3. Прошить: `pio run --target upload`
4. Мониторинг UART0/USB-CDC: `pio device monitor -b 115200`

## Структура проекта
- `platformio.ini` — конфигурация PlatformIO, окружение `p4_16mb` с PSRAM и LVGL.
- `src/main.cpp` — инициализация дисплея/тача, настройка LVGL и простой UI.
- `src/pins_config.h` — пины и разрешение дисплея (1024x600).
- `src/lv_conf.h` — параметры LVGL (используется `LV_CONF_INCLUDE_SIMPLE`).
- `src/lcd/`, `src/touch/` — драйверы JD9165 и GT911.

## Настройки и подстройка
- Калибровка тача задается в `src/main.cpp` в структуре `touch_calibration_t`; при необходимости скорректируйте минимальные/максимальные значения.
- Если нужен другой тип подключения к ПК, включите USB-CDC в `platformio.ini` (флаги `TINYUSB_CDC_ENABLED`/`ARDUINO_USB_CDC_ON_BOOT` закомментированы).
- Логи по умолчанию идут на UART0 и USB-CDC; уровни логирования настраиваются через `esp_log_level_set` в `setup()`.

## Полезно знать
- LVGL использует двойной буфер в PSRAM (`heap_caps_malloc`), поэтому требуется плата с PSRAM.
- Разрешение и пины заданы под JC1060P470C_I_W; при переносе на другую плату обновите `pins_config.h` и параметры дисплея.
