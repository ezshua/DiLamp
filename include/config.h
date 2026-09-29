// config.h — единая точка настройки DiLamp.
//
// Правила этого файла:
//
//  1. Зависимость только от <stdint.h>. Подключать Arduino.h НЕЛЬЗЯ: конфиг
//     читают чистые модули (эффекты, раскладка, настройки), которые не должны
//     зависеть от платформы. Все пины записаны числами.
//
//  2. Каждая настройка проходит два слоя: макрос DILAMP_* можно переопределить
//     через build_flags в platformio.ini, а код использует типизированную
//     константу k*. Значение по умолчанию и точка переопределения — рядом.
//
//  3. Числа по умолчанию подобраны под 20 диодов WS2812 и питание от MT3608
//     (docs/PLAN.md §4.3). Менять лимиты яркости без пересчёта тока нельзя:
//     20 диодов на полном белом дают 1,2 А, а MT3608 надёжно выдаёт ~1 А.

#ifndef DILAMP_CONFIG_H
#define DILAMP_CONFIG_H

#include <stdint.h>

// ===========================================================================
// Пины
// ===========================================================================
//
// Кнопки подтянуты INPUT_PULLUP внутри и замыкаются на GND.
// A0 на ATmega328P — это цифровой индекс 14; LGT8F328P в форм-факторе Nano
// имеет ту же раскладку, поэтому число переносится без изменений.

#ifndef DILAMP_LED_PIN
#  define DILAMP_LED_PIN 6
#endif

#ifndef DILAMP_BUTTON_NEXT_PIN
#  define DILAMP_BUTTON_NEXT_PIN 2
#endif

#ifndef DILAMP_BUTTON_PREV_PIN
#  define DILAMP_BUTTON_PREV_PIN 3
#endif

#ifndef DILAMP_BUTTON_PAUSE_PIN
#  define DILAMP_BUTTON_PAUSE_PIN 4
#endif

#ifndef DILAMP_BATTERY_ADC_PIN
#  define DILAMP_BATTERY_ADC_PIN 14  // A0
#endif

constexpr uint8_t kLedPin = DILAMP_LED_PIN;
constexpr uint8_t kButtonNextPin = DILAMP_BUTTON_NEXT_PIN;
constexpr uint8_t kButtonPrevPin = DILAMP_BUTTON_PREV_PIN;
constexpr uint8_t kButtonPausePin = DILAMP_BUTTON_PAUSE_PIN;
constexpr uint8_t kBatteryAdcPin = DILAMP_BATTERY_ADC_PIN;

// Скорость ленты, кГц. 800 — WS2812B (частый случай), 400 — классические WS2812
// и WS2812-2020. Неверная скорость даёт «мусор вместо цвета», а маркировка
// диодов пока не проверена (открытый вопрос в docs/TODO.md), поэтому значение
// вынесено сюда и меняется одной строкой.
//
// Здесь только число: config.h не знает про Adafruit_NeoPixel, перевод макросов
// библиотеки делает main.cpp.
constexpr uint16_t kStripKhz = 800;

// ===========================================================================
// Кольца
// ===========================================================================
//
// Два кольца вложены одно в другое и соединены последовательно. Геометрически
// большее кольцо — на 12 диодов, и оно внешнее; на 8 диодов — внутреннее
// (подтверждено владельцем).
//
// Направление обхода: два вложенных кольца, соединённые «одинаково», визуально
// вращаются В РАЗНЫЕ стороны — «волна» по внешнему идёт против часовой, пока
// по внутреннему по часовой. Чтобы направление совпадало, у одного кольца
// обход инвертирован. Какого именно — проверяется на железе (задача 20).
// Значения ниже гипотетические; правка = переставить две константы, код
// эффектов не затрагивается.

constexpr uint8_t kInnerCount = 8;                          // внутреннее кольцо
constexpr uint8_t kOuterCount = 12;                         // внешнее кольцо
constexpr uint8_t kTotalCount = kInnerCount + kOuterCount;  // 20

// Индексы в общей ленте, с которого начинается каждое кольцо. По умолчанию
// внешнее кольцо в цепочке идёт первым; если при сборке подключат наоборот —
// меняются kOuterStart и kInnerStart, больше ничего трогать не нужно.
constexpr uint8_t kOuterStart = 0;
constexpr uint8_t kInnerStart = kOuterStart + kOuterCount;

constexpr bool kOuterReversed = false;
constexpr bool kInnerReversed = true;

// Раскладка проверяется на этапе компиляции: кольца не должны выходить за
// пределы ленты и не должны перекрываться.
static_assert(kOuterStart + kOuterCount <= kTotalCount, "внешнее кольцо выходит за пределы ленты");
static_assert(kInnerStart + kInnerCount <= kTotalCount,
              "внутреннее кольцо выходит за пределы ленты");
static_assert(kInnerStart >= kOuterStart + kOuterCount || kOuterStart >= kInnerStart + kInnerCount,
              "кольца перекрываются в раскладке");

// ===========================================================================
// Яркость и ток
// ===========================================================================
//
// kPerPixelMax ограничивает яркость одного диода: при нём ток ленты около
// 0,75 А вместо 1,2 А на полном белом. Это единственная защита от перегрева
// boost-преобразователя, поэтому лимиты не «подкручиваются на глаз».

#ifndef DILAMP_PER_PIXEL_MAX
#  define DILAMP_PER_PIXEL_MAX 160
#endif

#ifndef DILAMP_DEFAULT_BRIGHTNESS
#  define DILAMP_DEFAULT_BRIGHTNESS 128
#endif

#ifndef DILAMP_MAX_BRIGHTNESS
#  define DILAMP_MAX_BRIGHTNESS 180
#endif

constexpr uint8_t kPerPixelMax = DILAMP_PER_PIXEL_MAX;
constexpr uint8_t kDefaultBrightness = DILAMP_DEFAULT_BRIGHTNESS;
constexpr uint8_t kMaxBrightness = DILAMP_MAX_BRIGHTNESS;

// Шаг яркости при удержании кнопки.
constexpr uint8_t kBrightnessStep = 16;

static_assert(kDefaultBrightness <= kMaxBrightness, "яркость по умолчанию выше максимальной");
static_assert(kPerPixelMax <= 255, "потолок яркости диода вне диапазона");

// ===========================================================================
// Тайминги
// ===========================================================================
//
// show() для 20 диодов держит прерывания выключенными ~600 мкс, поэтому темп
// 30 кадров/с: для медленных эффектов хватает, а метронод millis() почти не
// теряет тики (docs/PLAN.md §4.8).

constexpr uint32_t kFrameIntervalMs = 33;    // ~30 FPS
constexpr uint32_t kDebounceMs = 40;         // подавление дребезга кнопок
constexpr uint32_t kLongPressMs = 800;       // удержание = изменение яркости
constexpr uint32_t kVeryLongPressMs = 1500;  // пауза = мягкое выкл/вкл
constexpr uint32_t kCrossfadeMs = 300;       // переход между эффектами
constexpr uint32_t kStartupRampMs = 600;     // плавный старт без белой вспышки
constexpr uint32_t kPowerOffFadeMs = 800;    // мягкое угасание
constexpr uint32_t kBatteryPollMs = 1000;    // период опроса батареи

// ===========================================================================
// Диагностика и консоль
// ===========================================================================
//
// Буфер Serial на ATmega328P — 64 байта, при 115200 бод переполнение
// блокирует главный цикл. Logger пишет в кольцевой буфер и сливает его
// порциями, поэтому размеры буферов здесь фиксированы (docs/PLAN.md §4.9).

constexpr uint8_t kLogBufferSize = 128;         // кольцевой буфер лога
constexpr uint8_t kLogFlushBytesPerLoop = 32;   // сброс за итерацию цикла
constexpr uint8_t kCommandLineBufferSize = 48;  // строка команды с ЗАПРОСОМ
constexpr uint8_t kCommandMaxArgs = 4;          // команда + до трёх аргументов

// ===========================================================================
// Батарея
// ===========================================================================
//
// Напряжение снимается делителем 200 кОм / 100 кОм на A0. Делитель постоянно
// нагружает батарею на 14 мкА — для Li-ion это пренебрежимо.

constexpr uint16_t kBatteryDividerTopKiloOhm = 200;     // верхний резистор
constexpr uint16_t kBatteryDividerBottomKiloOhm = 100;  // нижний резистор

constexpr uint16_t kBatteryEmptyMilliVolt = 3300;  // ниже — гасим лампу
constexpr uint16_t kBatteryWarnMilliVolt = 3600;   // ниже — предупреждение в лог
constexpr uint16_t kBatteryFullMilliVolt = 4200;   // заряжена

// Опорное напряжение АЦП. При питании ленты от USB релька выше 5 В
// (через диод Шоттки) и измеряется около 5,12 В: ошибка 2,4 % на пороге
// разряда, что погрешности самого делителя не превышает.
constexpr uint16_t kAdcReferenceMilliVolt = 5000;

// Разрядность АЦП. ATmega328P — 10 бит, LGT8F328P — 12 бит. Задаётся в
// platformio.ini через build_flags; здесь значение по умолчанию, чтобы файл
// компилировался сам по себе.
#ifndef DILAMP_ADC_BITS
#  define DILAMP_ADC_BITS 10
#endif

constexpr uint8_t kAdcBits = DILAMP_ADC_BITS;

static_assert(kAdcBits == 10 || kAdcBits == 12, "поддерживаются только 10- и 12-разрядные АЦП");

// ===========================================================================
// Энкодер — задел на будущее (не подключён)
// ===========================================================================
//
// Переход на энкодер потребует освободить пины D2/D3/D4 под кнопки, поэтому
// значения лежат здесь, а не в коде эффектов (docs/PLAN.md §4.5).

constexpr uint8_t kEncoderPinA = 2;
constexpr uint8_t kEncoderPinB = 3;
constexpr uint8_t kEncoderPinButton = 4;
constexpr uint8_t kEncoderDetentCount = 24;  // щелчков на полный оборот

#endif  // DILAMP_CONFIG_H
