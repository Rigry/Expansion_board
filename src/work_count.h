#pragma once

#include <array>
#include "constant.h"
#include "timers.h"

namespace {
    constexpr auto remember_every        {60000};
    constexpr auto update_every_remember {60};
}

// каждая плата расширения будет считать свои лампы
// поэтому тут только 10
struct Minutes {
    std::array<uint32_t, 10> data {0};
};

// считает время работы каждой лампы в минутах
struct Work_count : TickSubscriber {
    int tick_cnt {0};
    Minutes minutes;
    const uint16_t& bad_lamps;
    std::array<uint16_t, glob::max_lamps>& hours;
    uint8_t& lamps_qty;

    Work_count (
          uint16_t& bad_lamps
        , std::array<uint16_t, glob::max_lamps>& hours
        , uint8_t& lamps_qty
    ) : bad_lamps {bad_lamps}, hours {hours}, lamps_qty{lamps_qty}
    {}

    Minutes* get_data() { return &minutes; }
    void start() { tick_subscribe(); }
    void stop() { tick_unsubscribe(); }

    void notify() override {
        if (++tick_cnt == remember_every) {
            tick_cnt = 0;
            for (auto i {0}; i < lamps_qty; i++) {
                minutes.data[i] += not ((bad_lamps >> i) & 0b1);
                hours[i] = get_hours(i);
            }
        }
    }

    uint16_t get_hours (int i) { return minutes.data[i] / update_every_remember; }
    void reset (int i) {
        minutes.data[i] = 0;
        hours[i] = 0;
    }

    void reset () {
        for (auto i {0}; i < 10; i++) {
            reset(i);
        }
    }

    void reset_by_mask (uint16_t mask) {
        for (auto i {0}; i < 10; i++) {
            if ((mask >> i) & 0b1)
                reset(i);
        }
    }
};