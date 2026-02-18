#pragma once

#include "i18n.h"

namespace settings {

// Russian translations
static const char* const STRINGS_RU[static_cast<size_t>(StringID::COUNT)] = {
    // Common
    [static_cast<size_t>(StringID::OK)] = "OK",
    [static_cast<size_t>(StringID::CANCEL)] = "Отмена",
    [static_cast<size_t>(StringID::BACK)] = "Назад",
    [static_cast<size_t>(StringID::SAVE)] = "Сохранить",
    [static_cast<size_t>(StringID::DELETE)] = "Удалить",
    [static_cast<size_t>(StringID::CONNECT)] = "Подключиться",
    [static_cast<size_t>(StringID::DISCONNECT)] = "Отключиться",
    [static_cast<size_t>(StringID::FORGET)] = "Забыть",
    [static_cast<size_t>(StringID::SCANNING)] = "Поиск...",
    [static_cast<size_t>(StringID::CONNECTING)] = "Подключение...",
    [static_cast<size_t>(StringID::CONNECTED)] = "Подключено",
    [static_cast<size_t>(StringID::DISCONNECTED)] = "Отключено",
    [static_cast<size_t>(StringID::ERROR)] = "Ошибка",
    [static_cast<size_t>(StringID::LOADING)] = "Загрузка...",
    
    // Settings Home
    [static_cast<size_t>(StringID::SETTINGS_TITLE)] = "Настройки",
    [static_cast<size_t>(StringID::SETTINGS_WIFI)] = "Wi-Fi",
    [static_cast<size_t>(StringID::SETTINGS_DISPLAY)] = "Экран",
    [static_cast<size_t>(StringID::SETTINGS_TIME)] = "Время",
    [static_cast<size_t>(StringID::SETTINGS_LANGUAGE)] = "Язык",
    [static_cast<size_t>(StringID::SETTINGS_SYSTEM)] = "Система",

    // Apps
    [static_cast<size_t>(StringID::APP_BOX_TEST)] = "Тест боксов",
    [static_cast<size_t>(StringID::APP_CLOCK)] = "Часы",
    [static_cast<size_t>(StringID::APP_SETTINGS)] = "Настройки",
    [static_cast<size_t>(StringID::APP_GALLERY)] = "Галерея",
    [static_cast<size_t>(StringID::APP_MUSIC)] = "Музыка",
    [static_cast<size_t>(StringID::APP_FILES)] = "Файлы",
    [static_cast<size_t>(StringID::APP_CALCULATOR)] = "Калькулятор",
    
    // WiFi
    [static_cast<size_t>(StringID::WIFI_TITLE)] = "Wi-Fi",
    [static_cast<size_t>(StringID::WIFI_STATUS)] = "Статус",
    [static_cast<size_t>(StringID::WIFI_SCAN)] = "Поиск сетей",
    [static_cast<size_t>(StringID::WIFI_SAVED_NETWORKS)] = "Сохраненные сети",
    [static_cast<size_t>(StringID::WIFI_ENTER_PASSWORD)] = "Введите пароль",
    [static_cast<size_t>(StringID::WIFI_PASSWORD)] = "Пароль",
    [static_cast<size_t>(StringID::WIFI_SHOW_PASSWORD)] = "Показать пароль",
    [static_cast<size_t>(StringID::WIFI_CONNECTING)] = "Подключение...",
    [static_cast<size_t>(StringID::WIFI_CONNECTED)] = "Подключено",
    [static_cast<size_t>(StringID::WIFI_DISCONNECTED)] = "Отключено",
    [static_cast<size_t>(StringID::WIFI_FAILED)] = "Ошибка подключения",
    [static_cast<size_t>(StringID::WIFI_SSID)] = "Сеть",
    [static_cast<size_t>(StringID::WIFI_IP_ADDRESS)] = "IP адрес",
    [static_cast<size_t>(StringID::WIFI_SIGNAL)] = "Сигнал",
    [static_cast<size_t>(StringID::WIFI_NO_NETWORKS)] = "Сети не найдены",
    [static_cast<size_t>(StringID::WIFI_AUTO_CONNECT)] = "Автоподключение",
    
    // Display
    [static_cast<size_t>(StringID::DISPLAY_TITLE)] = "Экран",
    [static_cast<size_t>(StringID::DISPLAY_BRIGHTNESS)] = "Яркость",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT)] = "Таймаут подсветки",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_15S)] = "15 сек",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_30S)] = "30 сек",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_1M)] = "1 мин",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_5M)] = "5 мин",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_NEVER)] = "Никогда",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION)] = "Поворот экрана",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION_0)] = "0°",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION_90)] = "90°",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION_180)] = "180°",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION_270)] = "270°",
    
    // Time
    [static_cast<size_t>(StringID::TIME_TITLE)] = "Время",
    [static_cast<size_t>(StringID::TIME_CURRENT)] = "Текущее время",
    [static_cast<size_t>(StringID::TIME_DATE)] = "Дата",
    [static_cast<size_t>(StringID::TIME_TIME)] = "Время",
    [static_cast<size_t>(StringID::TIME_TIMEZONE)] = "Часовой пояс",
    [static_cast<size_t>(StringID::TIME_NTP_ENABLED)] = "Синхронизация NTP",
    [static_cast<size_t>(StringID::TIME_NTP_SERVER)] = "Сервер NTP",
    [static_cast<size_t>(StringID::TIME_SYNC_NOW)] = "Синхронизировать",
    [static_cast<size_t>(StringID::TIME_MANUAL_SET)] = "Установить вручную",
    [static_cast<size_t>(StringID::TIME_YEAR)] = "Год",
    [static_cast<size_t>(StringID::TIME_MONTH)] = "Месяц",
    [static_cast<size_t>(StringID::TIME_DAY)] = "День",
    [static_cast<size_t>(StringID::TIME_HOUR)] = "Час",
    [static_cast<size_t>(StringID::TIME_MINUTE)] = "Минута",
    [static_cast<size_t>(StringID::TIME_SECOND)] = "Секунда",
    
    // Language
    [static_cast<size_t>(StringID::LANGUAGE_TITLE)] = "Язык",
    [static_cast<size_t>(StringID::LANGUAGE_SELECT)] = "Выберите язык",
    [static_cast<size_t>(StringID::LANGUAGE_RUSSIAN)] = "Русский",
    [static_cast<size_t>(StringID::LANGUAGE_ENGLISH)] = "English",
    [static_cast<size_t>(StringID::LANGUAGE_FORMAT_24H)] = "24 часа",
    [static_cast<size_t>(StringID::LANGUAGE_FORMAT_12H)] = "12 часов (AM/PM)",
    
    // System
    [static_cast<size_t>(StringID::SYSTEM_TITLE)] = "Система",
    [static_cast<size_t>(StringID::SYSTEM_FIRMWARE_VERSION)] = "Версия прошивки",
    [static_cast<size_t>(StringID::SYSTEM_UPTIME)] = "Время работы",
    [static_cast<size_t>(StringID::SYSTEM_FREE_HEAP)] = "Свободно памяти",
    [static_cast<size_t>(StringID::SYSTEM_FREE_PSRAM)] = "Свободно PSRAM",
    [static_cast<size_t>(StringID::SYSTEM_RESET_SETTINGS)] = "Сброс настроек",
    [static_cast<size_t>(StringID::SYSTEM_REBOOT)] = "Перезагрузка",
    [static_cast<size_t>(StringID::SYSTEM_RESET_CONFIRM)] = "Сбросить все настройки?",
    [static_cast<size_t>(StringID::SYSTEM_REBOOT_CONFIRM)] = "Перезагрузить устройство?",
};

} // namespace settings
