#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "aseba_can.h"
#include "config.h"
#include "uart.h"

#define ASEBA_TYPE_SMALL_PACKET 0x300

// CAN1 PD0(RX) PD1(TX)

void aseba_can_init(void)
{
    rcc_periph_clock_enable(RCC_CAN1);
    rcc_periph_clock_enable(RCC_GPIOD);

    gpio_set_af(GPIOD, GPIO_AF9, GPIO0 | GPIO1);
    gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0 | GPIO1);

    /* CAN cell init.
     * Setting the bitrate to 1MBit. APB1 = 42MHz,
     * prescaler = 2 -> 21MHz time quanta frequency.
     * 1tq sync + 12tq bit segment1 (TS1) + 8tq bit segment2 (TS2) =
     * 21time quanta per bit period, therefor 21MHz/21 = 1MHz
     */
    if (can_init(CAN1,          // Interface
             false,             // Time triggered communication mode.
             true,              // Automatic bus-off management.
             false,             // Automatic wakeup mode.
             false,             // No automatic retransmission.
             false,             // Receive FIFO locked mode.
             false,             // Transmit FIFO priority.
             CAN_BTR_SJW_1TQ,   // Resynchronization time quanta jump width
             CAN_BTR_TS1_12TQ,  // Time segment 1 time quanta width
             CAN_BTR_TS2_8TQ,   // Time segment 2 time quanta width
             2,                 // Prescaler
             false,             // Loopback
             false)) {          // Silent
        uart_puts("ERROR: CAN init failed\n");
    }

    can_filter_id_mask_32bit_init(
        CAN1,
        0,      // nr
        0,      // id
        0,      // mask
        0,      // fifo
        true    // enable
    ); // match any id
}


int aseba_can_send(const uint16_t *msg, int len, int id)
{
    if (can_transmit(CAN1, id | ASEBA_TYPE_SMALL_PACKET, false, false, len*2, (uint8_t*)msg) >= 0) {
        // uart_puts("x");
        return 0;
    } else {
        uart_puts("ERROR sending frame\n");
        return -1;
    }
}

int aseba_can_receive(uint16_t *msg)
{
    if (CAN_RF0R(CAN1) & CAN_RF0R_FMP0_MASK) {
        uint32_t id, fmi;
        bool ext, rtr;
        uint8_t length;
        uint8_t data[8];
        can_receive(CAN1, 0, true, &id, &ext, &rtr, &fmi, &length, data);
        // uart_puts("can message received\n");
        if (length % 2 != 0) {
            return -1;
        }
        // if ((id & 0x300) != ASEBA_TYPE_SMALL_PACKET) {
        //     return -1;
        // }
        int aseba_length = length / 2;
        int i;
        for (i = 0; i < aseba_length; i++) {
            msg[i] = data[i*2] + (data[i*2 + 1]<<8); // aseba data is little endian
        }
        return aseba_length;
    } else {
        return -1;
    }
}
