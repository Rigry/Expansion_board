#pragma once

#include "pin.h"
#include "constant.h"
#include "timers.h"
#include <bitset>

// TODO знаю что плохо, но спешил
template<class T>
auto from_tuple(T in) {
    return std::array{
        &std::get<0>(in),
        &std::get<1>(in),
        &std::get<2>(in),
        &std::get<3>(in),
        &std::get<4>(in),
        &std::get<5>(in),
        &std::get<6>(in),
        &std::get<7>(in),
        &std::get<8>(in),
        &std::get<9>(in)
    };
}

class Lamps : TickSubscriber {
public:
    template<class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10>
    static Lamps& make(uint16_t& bad_lamps, uint8_t& lamps_qty)
    {
        auto pins = make_pins<
            mcu::PinMode::Input,
            P1,P2,P3,P4,P5,P6,P7,P8,P9,P10
        >();
        static auto result = Lamps{
              from_tuple(pins)
            , bad_lamps
            , lamps_qty
        };
        return result;
    }


private:
    const std::array<Pin*, 10> pins;
    uint16_t& bad_lamps;
    const uint8_t& lamps_qty;

    Lamps(
          std::array<Pin*, 10> pins
        , uint16_t& bad_lamps
        , uint8_t& lamps_qty
    ) : pins {pins}, bad_lamps {bad_lamps}, lamps_qty{lamps_qty}
    {
        tick_subscribe();
    }

    void notify() override {
        // заполняем только первые 10
        uint16_t tmp{0};
        // TODO сюда бы пинлист из 2-ой библиотеки
        for (auto i{0}; i < lamps_qty; i++) {
            tmp |= *pins[i] << i;
        }
        bad_lamps = tmp;
    }
};


template<class T>
auto from_tuple_4(T in) {
    return std::array{
        &std::get<0>(in),
        &std::get<1>(in),
        &std::get<2>(in),
        &std::get<3>(in)
    };
}

class Value {

    const std::array<Pin*, 4> pins;
    uint8_t value{0};

    Value(
          std::array<Pin*, 4> pins
    ) : pins {pins}
    {}

public:

    template<class P1, class P2, class P3, class P4>
    static Value& make()
    {
        auto pins = make_pins<
            mcu::PinMode::Input,
            P1,P2,P3,P4
        >();
        static auto result = Value{
              from_tuple_4(pins)
        };
        return result;
    }

    uint8_t operator() () {
        uint8_t tmp{0};
        for (auto i{0}; i < 4; i++) {
            tmp |= *pins[i] << i;
        }
        value = tmp;

        return value;
    }

};