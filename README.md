# Embedded Systems Lab

ESP32 firmware projects built using ESP-IDF and C, developed as part of a structured 32-week embedded systems and VLSI learning roadmap targeting roles at Tesla, Waymo, Bosch, and TI.

---

## S01 — Power Rail Monitor

**Platform:** ESP32 DevKit C | **Toolchain:** ESP-IDF | **Language:** C + Python

A 3-channel ADC monitoring system to track power rail health and board temperature in real time.

### Channels
| Channel | GPIO | Signal |
|---------|------|--------|
| CH1 | GPIO34 | VCC rail |
| CH2 | GPIO35 | Secondary 3.3V rail |
| CH3 | GPIO32 | Board temperature (LM35) |

### Features (planned / in progress)
- [x] Step 1: Single-channel raw ADC read over UART (complete)
- [ ] Step 2: OOP refactor — `ADC_Channel_t` struct, per-channel min/max, ring buffer rolling average
- [ ] Step 3: 3-channel integration with UART JSON output
- [ ] Step 4: Python live plot + CSV logger

### Toolchain
- ESP-IDF v5.x (adc_oneshot API)
- Wokwi for simulation; hardware validation on real ESP32 pending Step 2 completion

---

## Roadmap
More projects will be added across phases covering FreeRTOS, STM32 LL drivers, Verilog/FPGA, and VLSI design.
