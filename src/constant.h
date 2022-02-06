#pragma once

uint8_t lamps;

struct Count {
    uint16_t on;
    uint16_t reset_all;
    uint16_t reset_one;
    uint16_t reset_log;
};

struct Flags {
    bool us_on        : 1;
    bool uv_on        : 1;
    bool uv_low_level : 1;
    bool overheat     : 1;
    bool us_started   : 1;
    bool uv_started   : 1;
    bool bad_lamps    : 1;
    uint16_t          : 9;
    bool is_alarm() { return bad_lamps or overheat or uv_low_level; }
};


namespace glob {
    auto constexpr max_lamps       {10};
    auto constexpr password        {1207};
}