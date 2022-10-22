/*
   Проект простого хлопкового выключателя света.
   Использует амлитудный анализ сигнала с микрофонного модуля MAX8914.
   - Настраиваемое количество хлопков для включения / выключения.
   - Настраиваемое временное окно между хлопками (фильтрация постороронних резких звуков).
   - Можно управлять любой нагрузкой при помощи реле, mosfet и т.д.
*/

/* ---- Отладка и сторожевой таймер---- */
#define DEBUG_EN  0       // Отладка по Serial, 1 - вкл / 0 - выкл
#define WDT_EN    0       // Сторожевой таймер, 1 - вкл / 0 - выкл

/* ---- Настройка пинов ---- */
#define MIC_PIN   A1      // Аналоговый пин микрофона
#define REL_PIN   4       // Цифровой пин реле / mosfet

/* ---- Настройка распознавания (необходимо подобрать под себя!) ---- */
#define CLAP_WINDOW_H 600 // Макс. значение периода между хлопками (мс)
#define CLAP_WINDOW_L 250 // Мин. значение периода между хлопками (мс)
#define CLAP_TIMEOUT  600 // Таймаут распознования хлопков (мс)
#define CLAP_LVL      180 // Пороговый уровень относительной громкости хлопков
#define CLAP_NUM      2   // Количество хлопков (не менее 2х)

#include <VolAnalyzer.h>
#include <GyverWDT.h>

VolAnalyzer VA (MIC_PIN); // Объект анализатора громкости

#if (DEBUG_EN)
#define DBG_PRINT(x)    (Serial.print(x))
#define DBG_PRINTLN(x)  (Serial.println(x))
#else
#define DBG_PRINT(x)
#define DBG_PRINTLN(x)
#endif

void setup() {
#if (WDT_EN)                                       // Если WDT активен
  Watchdog.disable();                              // Всегда отключаем watchdog при старте
#endif

  pinMode(REL_PIN, OUTPUT);                        // Пин реле как выход
  digitalWrite(REL_PIN, LOW);                      // Выключаем реле

#if defined (INTERNAL2V56)                         // Если доступно опорное 2.56V
  analogReference(INTERNAL2V56);                   // Выбираем его
#endif

#if (DEBUG_EN)                                     // Если отладка активна
  Serial.begin(9600);                              // Подключаем Serial
#endif

#if (WDT_EN)                                       // Если WDT активен
  Watchdog.enable(RST_MODE, WDT_TIMEOUT_512MS);    // Режим сторжевого сброса , таймаут ~0.5 секунды
#endif
}

void loop() {
  static long timer = millis();                     // Переменная таймера
  static uint8_t count = 0;                         // Счетчик хлопков
  static bool enable = false;                       // Флаг состояния реле

  if (VA.tick()) {                                  // Опрос анализатора громкости
    if (VA.pulse() && (VA.getMax() >= CLAP_LVL)) {  // Если скачок громкости + пик выше порога = возможный хлопок
      if ((millis() - timer) >= CLAP_WINDOW_L && (millis() - timer) <= CLAP_WINDOW_H) {
        count++;                                    // Время между хлопками попало в интервал - увеличиваем счетчик
        DBG_PRINT("Clap detected: ");               // Выводим отладочную инфу
        DBG_PRINTLN(count);                         // И текущее значение счетчика
      } else count = 0;                             // Если интервал нарушен - все в топку
      timer = millis();                             // Обновляем таймер
    }
  }

  if (count and millis() - timer >= CLAP_TIMEOUT) {  // Если хлопки уже давно не происходят (таймаут)
    DBG_PRINTLN("Clap timeout");                     // Выводим сообщение
    if (count == (CLAP_NUM - 1)) {                   // Если нахлопали нужное количество раз
      enable = !enable;                              // Инвертируем флаг состояния реле
      digitalWrite(REL_PIN, enable);                 // Управляем пином реле
      DBG_PRINTLN(enable ? "ENABLE" : "DISABLE");    // Выводим состояние реле
    } count = 0;                                     // Сбрасываем счетчик
  }

#if (WDT_EN)                                         // Если WDT активен
  Watchdog.reset();                                  // Cброс watchdog - устройство не зависло
#endif
}
