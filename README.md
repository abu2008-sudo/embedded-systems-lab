# Embedded Systems Lab
ESP32 firmware projects built using ESP-IDF and C

---

## S01 — Power Rail Monitor
**Platform:** ESP32 DevKit C | **Toolchain:** ESP-IDF (not Arduino) | **Language:** C + Python

A 3-channel ADC monitoring system tracking power rail voltages in real time, with both
short-term (range-based) and permanent (all-time) statistics per channel.

### Channels
| Channel | GPIO | Signal |
|---------|------|--------|
| CH1 | GPIO34 | VCC rail |
| CH2 | GPIO35 | Secondary 3.3V rail |
| CH3 | GPIO32 | Board temperature (LM35) |

### Architecture
- `ADC_Channel_t` — OOP-style struct per channel, holding a circular history buffer
- **Flexible buffer size** — each channel can request its own history window size,
  capped safely at a compile-time `MAX_HISTORY` (32) to avoid heap allocation
- **Range-based stats** (`range_min`, `range_max`, `avg`) — recomputed each cycle from
  only the currently valid buffer entries
- **Global stats** (`global_min`, `global_max`) — persist for the entire runtime,
  never reset, capturing all-time extremes even after they've cycled out of the buffer

### Validation
Firmware logic validated with a software-in-the-loop (SIL) test before hardware deployment:
- Core stats logic ported to a portable, non-ESP-IDF C file (`sil_test/s01_sil.c`)
- Python script (`sil_test/sil_stimulus.py`) feeds a deliberate voltage spike through
  the pipeline via stdin
- Confirms `range_max` correctly drops back to normal once the spike cycles out of the
  buffer, while `global_max` permanently retains it — see `sil_test/sample_output.txt`

### Status
- [x] Single-channel raw ADC read over UART
- [x] OOP refactor — `ADC_Channel_t` struct, range-based + global min/max, flexible circular buffer
- [x] SIL validation of stats logic (spike test)
- [ ] JSON UART output (deprioritized — plain UART logging in place)
- [ ] Circuit-level simulation (LTspice)
- [ ] Hardware-in-the-loop validation on physical ESP32
- [ ] Python live plot + CSV logger

### Toolchain
- ESP-IDF v5.x (`adc_oneshot` API), FreeRTOS task structure
- Simulation: GCC + Python SIL harness (replaced Wokwi for firmware logic testing)
- Hardware validation: pending physical ESP32 board

---

## Roadmap
More projects will be added across phases covering FreeRTOS, STM32 LL drivers,
