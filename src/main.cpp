// main.cpp — тестовый паттерн, задача 4.
//
// Задача паттерна — проверить прошивку и линию данных ДО написания логики:
//
//   • оба кольца горят разным цветом → видно, что DATA дошла до обоих и
//     видно, какое кольцо где;
//   • белый маркер идёт по каждому кольцу и мигает 1 раз в секунду →
//     видно направление обхода (задача 19) и есть опора для секундомера
//     при сверке частоты (задача 5);
//   • F_CPU печатается один раз при старте.
//
// Правила (AGENTS.md §3.2): без delay(), show() только при изменении кадра,
// в цикле Serial не используется.
//
// Раскладка «кольцо + индекс» → индекс ленты здесь временная: её забирает
// RingLayout (задача 8) вместе с нормализацией направления обхода.

#include <Arduino.h>

#include <Adafruit_NeoPixel.h>

#include "config.h"

namespace {

// Adafruit_NeoPixel сам выделяет буфер через malloc: 20 × 3 = 60 байт из кучи.
// При 2 КБ SRAM это учитывается в бюджете памяти (задача 17).
constexpr int kStripKind = NEO_GRB + (kStripKhz == 800 ? NEO_KHZ800 : NEO_KHZ400);

Adafruit_NeoPixel gStrip(kTotalCount, kLedPin, kStripKind);

constexpr uint32_t kStepMs = 1000;  // маркер сдвигается на одну позицию
constexpr uint32_t kPhaseMs = 500;  // маркер то горит, то нет

// Временный маппинг, заменяется RingLayout (задача 8).
uint8_t innerIndex(uint8_t i) {
  return kInnerReversed ? (kInnerStart + kInnerCount - 1 - i) : (kInnerStart + i);
}

uint8_t outerIndex(uint8_t i) {
  return kOuterReversed ? (kOuterStart + kOuterCount - 1 - i) : (kOuterStart + i);
}

// Тёплый внутри, холодный снаружи: кольца различаются даже сквозь
// рассеиватель. Яркость ограничена kPerPixelMax — потолок по току (§4.3).
void paintBase() {
  for (uint8_t i = 0; i < kInnerCount; ++i) {
    gStrip.setPixelColor(innerIndex(i), Adafruit_NeoPixel::Color(0, 0, kPerPixelMax));
  }
  for (uint8_t i = 0; i < kOuterCount; ++i) {
    gStrip.setPixelColor(outerIndex(i), Adafruit_NeoPixel::Color(0, 0, kPerPixelMax));
  }
}

// Индекс последней отрисованной фазы. Пока он тот же — кадр не изменился,
// и вызывать show() незачем (§3.2).
uint32_t gLastPhase = 0xFFFFFFFFu;

}  // namespace

void setup() {
  Serial.begin(115200);

  // Разовая диагностика при старте. В цикле Serial не используется: буфер
  // на ATmega328P — 64 байта, и при 115200 бод переполнение блокирует цикл.
  Serial.println(F("=== DiLamp, тестовый паттерн ==="));
  Serial.print(F("F_CPU = "));
  Serial.println(F_CPU);
  Serial.print(F("диодов всего = "));
  Serial.println(kTotalCount);
  Serial.print(F("внутреннее = "));
  Serial.print(kInnerCount);
  Serial.print(F(", внешнее = "));
  Serial.println(kOuterCount);
  Serial.print(F("скорость ленты = "));
  Serial.print(kStripKhz);
  Serial.println(F(" кГц"));
  Serial.println(F("маркер мигает 1 раз в секунду — сверяйте по секундомеру"));

  gStrip.begin();
  gStrip.clear();
  gStrip.show();

  paintBase();
  gStrip.show();
}

void loop() {
  const uint32_t now = millis();
  const uint32_t phase = now / kPhaseMs;

  if (phase == gLastPhase) {
    return;
  }
  gLastPhase = phase;

  // Маркер виден первые 500 мс каждой секунды и не виден вторые 500.
  const bool markerVisible = (phase % 2) == 0;

  paintBase();

  if (markerVisible) {
    const uint32_t step = now / kStepMs;
    const uint8_t v = Adafruit_NeoPixel::Color(kPerPixelMax, kPerPixelMax, kPerPixelMax);
    gStrip.setPixelColor(innerIndex(step % kInnerCount), v);
    gStrip.setPixelColor(outerIndex(step % kOuterCount), v);
  }

  gStrip.show();
}
