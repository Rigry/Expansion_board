#define STM32F103xB
#define F_OSC   8000000UL
#define F_CPU   72000000UL

#define HEAP_SIZE 1024

#include "init_clock.h"
#include "constant.h"
#include "timers.h"
#include "literals.h"
#include "modbus_slave.h"
#include "modbus_master.h"
#include "hysteresis.h"
#include "utils.h"
#include "flash.h"
#include "safe_flash.h"
#include "lamps.h"
#include "work_count.h"


/// эта функция вызывается первой в startup файле
extern "C" void init_clock() { init_clock<F_OSC, F_CPU>(); }

using TX  = mcu::PC10;
using RX  = mcu::PC11;
using RTS = mcu::PA15;

using A0 = mcu::PB4;
using A1 = mcu::PB5;
using A2 = mcu::PB6;
using A3 = mcu::PB7;

using J0 = mcu::PB8;
using J1 = mcu::PB9;
using J2 = mcu::PB0;
using J3 = mcu::PB1;

using FACTORY = mcu::PC3;

using EPRA1  = mcu::PA0;
using EPRA2  = mcu::PA1;
using EPRA3  = mcu::PA2;
using EPRA4  = mcu::PA3;
using EPRA5  = mcu::PA4;
using EPRA6  = mcu::PA5;
using EPRA7  = mcu::PA6;
using EPRA8  = mcu::PA7;
using EPRA9  = mcu::PC4;
using EPRA10 = mcu::PC5;



int main()
{
    struct Flash_data {

        uint16_t factory_number = 0;
        UART::Settings uart_set = {
            .parity_enable  = false,
            .parity         = USART::Parity::even,
            .data_bits      = USART::DataBits::_8,
            .stop_bits      = USART::StopBits::_1,
            .baudrate       = USART::Baudrate::BR9600,
            .res            = 0
        };
        uint8_t  modbus_address = 1;
        uint8_t  lamps = 5;
        uint8_t  model_number = 0;
        // Count count = {
        //     .on        = 0,
        //     .reset_all = 0,
        //     .reset_one = 0,
        //     .reset_log = 0
        // };
    } flash;

    [[maybe_unused]] auto _ = Flash_updater<
          mcu::FLASH::Sector::_127
        , mcu::FLASH::Sector::_126
    >::make (&flash);

    struct In_regs {
        UART::Settings uart_set;         // 0
        uint16_t password;               // 1
        uint16_t factory_number;         // 3
        uint16_t reset_hours;            // 4
        uint8_t  n_lamp;                 // 5
    }__attribute__((packed));

    struct Out_regs {
        uint16_t       device_code;        // 0
        uint16_t       factory_number;     // 1
        UART::Settings uart_set;           // 2
        uint16_t       modbus_address;     // 3
        Flags          work_flags;         // 4
        uint8_t        lamps;              // 5
        uint16_t       bad_lamps;          // 6
        uint16_t       hours;              // 7
    }; // __attribute__((packed)); // TODO error: cannot bind packed field 


    auto address = Value::make<A0, A1, A2, A3>();
    flash.modbus_address = address();

    auto qty_lamps = Value::make<J0, J1, J2, J3>();
    flash.lamps = qty_lamps();

    
    // неудобно отлаживать, потому volatile
    volatile decltype(auto) modbus_slave = Modbus_slave<In_regs, Out_regs>::make <
          mcu::Periph::USART3
        , TX
        , RX
        , RTS
    >(flash.modbus_address, flash.uart_set);

    std::array<uint16_t, glob::max_lamps> hours{0};

    // подсчёт часов работы
    auto work_count = Work_count{
          modbus_slave.outRegs.bad_lamps
        , hours
        , flash.lamps
    };

    [[maybe_unused]] auto __ = Safe_flash_updater<
          mcu::FLASH::Sector::_89
        , mcu::FLASH::Sector::_115
    >::make (work_count.get_data());

    #define ADR(reg) GET_ADR(In_regs, reg)
    modbus_slave.outRegs.device_code       = 8;
    modbus_slave.outRegs.factory_number    = flash.factory_number;
    modbus_slave.outRegs.uart_set          = flash.uart_set;
    modbus_slave.outRegs.modbus_address    = flash.modbus_address;
    modbus_slave.outRegs.lamps             = flash.lamps;
    modbus_slave.arInRegsMax[ADR(uart_set)]= 0x0F;

    auto& work_flags = modbus_slave.outRegs.work_flags;

    // Определение плохих ламп
    Lamps::make<
        EPRA1,EPRA2,EPRA3,EPRA4,EPRA5,EPRA6,EPRA7,EPRA8,EPRA9,EPRA10
    >(modbus_slave.outRegs.bad_lamps, flash.lamps);

    while (1) {
        modbus_slave( [&](auto registr){
            static bool unblock = false;
            switch (registr) {
                case ADR(uart_set):
                    flash.uart_set
                        = modbus_slave.outRegs.uart_set
                        = modbus_slave.inRegs.uart_set;
                break;
                case ADR(password):
                    unblock = modbus_slave.inRegs.password == 1207;
                break;
                case ADR(factory_number):
                    if (unblock) {
                        unblock = false;
                        flash.factory_number 
                            = modbus_slave.outRegs.factory_number
                            = modbus_slave.inRegs.factory_number;
                    }
                    unblock = true;
                break;
                case ADR(reset_hours): // TODO без плат расширения
                    work_count.reset_by_mask(modbus_slave.inRegs.reset_hours);
                break;
                case ADR(n_lamp):
                    uint8_t lamp{modbus_slave.inRegs.n_lamp};
                    lamp = lamp > 9 ? 9 : lamp;
                    modbus_slave.outRegs.hours
                        = work_count.get_hours(lamp);
                break;
            } // switch
        });

        // work_flags.bad_lamps = work_flags.uv_on and modbus_slave.outRegs.bad_lamps;

        __WFI();
    }
}
