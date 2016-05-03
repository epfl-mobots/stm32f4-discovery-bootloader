#include <stdint.h>
#include <stdbool.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>
#include "aseba_can.h"
#include "timeout_timer.h"
#include "aseba_flash.h"
#include "uart.h"
#include "config.h"


#define ASEBA_CMD_RESET         0x8000 // ask the bootloader to reset
#define ASEBA_CMD_READ_PAGE     0x8001 // ask the bootloader to read a page and send it back
#define ASEBA_CMD_WRITE_PAGE    0x8002 // ask the bootloader to write a page
#define ASEBA_CMD_PAGE_DATA     0x8003 // CAN message containing data from host
#define ASEBA_PUSH_DESCRIPTION  0x8004 // CAN message containing bootloader description
#define ASEBA_PUSH_PAGE_DATA    0x8005 // CAN message containing data to host
#define ASEBA_PUSH_ACK          0x8006 // CAN message containing return value

// codes for aseba_reply()
#define ASEBA_REPLY_OK                  0x0
#define ASEBA_REPLY_INVALID_SIZE        0x1
#define ASEBA_REPLY_PROGRAM_FAILED      0x2
#define ASEBA_REPLY_NOT_PROGRAMMING     0x3
#define ASEBA_REPLY_INVALID_VALUE       0x4


typedef struct {
    bool programming_mode;
    int current_page;
    int current_word_in_page;
} aseba_bootloader_context_t;


static uint16_t aseba_page_buffer[ASEBA_PAGE_SIZE_IN_WORDS];

static __attribute__((section(".noinit"))) uint64_t boot_magic_value;
#define BOOT_MAGIC_VALUE_RUN_APP 0xdb3c9869254dc8bc

void run_app(void); // defined in run_app.s

void check_run_application(void)
{
    if (boot_magic_value == BOOT_MAGIC_VALUE_RUN_APP) {
        boot_magic_value = 0;
        run_app();
    }
}


void reboot_to_application(void)
{
    boot_magic_value = BOOT_MAGIC_VALUE_RUN_APP;
    scb_reset_system();
}


void aseba_bootloader_context_init(aseba_bootloader_context_t *ctx)
{
    ctx->programming_mode = false;
    ctx->current_page = 0;
    ctx->current_word_in_page = 0;
}


void aseba_reply(uint16_t error_code)
{
    uint16_t msg[2];
    msg[0] = ASEBA_PUSH_ACK;
    msg[1] = error_code;
    aseba_can_send(msg, 2);
}


void send_page(uint16_t page_nbr)
{
    uint16_t msg[3];
    msg[0] = ASEBA_PUSH_PAGE_DATA;
    int word;
    for (word = 0; word < ASEBA_PAGE_SIZE_IN_WORDS; word+=2) {
        msg[1] = aseba_flash_read_word(page_nbr, word);
        msg[2] = aseba_flash_read_word(page_nbr, word+1);
        aseba_can_send(msg, 3);
    }
}


int aseba_cmd_exec(aseba_bootloader_context_t *ctx, uint16_t cmd, uint16_t *payload, int len)
{
    switch (cmd) {
    case ASEBA_CMD_RESET:
        aseba_reply(ASEBA_REPLY_OK);
        uart_puts("jumping to application\n");
        reboot_to_application();
        break;
    case ASEBA_CMD_READ_PAGE:
        // uart_puts("command read page\n");
        if (len != 1) {
            aseba_reply(ASEBA_REPLY_INVALID_SIZE);
            return -1;
        }
        send_page(payload[0]);
        break;
    case ASEBA_CMD_WRITE_PAGE:
        uart_puts("command write page\n");
        if (len != 1) {
            aseba_reply(ASEBA_REPLY_INVALID_SIZE);
            return -1;
        }
        int page_nbr = payload[0] - ASEBA_FIRST_PAGE;

        if (page_nbr > ASEBA_AVAILABLE_PAGES) {
            aseba_reply(ASEBA_REPLY_INVALID_VALUE);
            return -1;
        }
        ctx->programming_mode = true;
        ctx->current_page = page_nbr;
        ctx->current_word_in_page = 0;
        aseba_flash_erase_page(page_nbr);
        aseba_reply(ASEBA_REPLY_OK);
        break;
    case ASEBA_CMD_PAGE_DATA:
        // uart_puts("command page data\n");
        if (len != 2) {
            aseba_reply(ASEBA_REPLY_INVALID_SIZE);
            return -1;
        }
        if (!ctx->programming_mode) {
            aseba_reply(ASEBA_REPLY_NOT_PROGRAMMING);
            return -1;
        }
        aseba_page_buffer[ctx->current_word_in_page++] = payload[0];
        aseba_page_buffer[ctx->current_word_in_page++] = payload[1];
        if (ctx->current_word_in_page == ASEBA_PAGE_SIZE_IN_WORDS) {
            uart_puts("full page received\n");
            aseba_flash_write_page(ctx->current_page, aseba_page_buffer);
            aseba_reply(ASEBA_REPLY_OK);
        }
        break;
    default:
        // uart_puts("ERROR invalid command\n");
        break;
    }
    return 0;
}


void aseba_send_descr(void)
{
    uint16_t msg[4];
    msg[0] = ASEBA_PUSH_DESCRIPTION;
    msg[1] = ASEBA_PAGE_SIZE; // page size
    msg[2] = ASEBA_FIRST_PAGE; // page start
    msg[3] = ASEBA_AVAILABLE_PAGES; // page count
    aseba_can_send(msg, 4);
}


int main(void)
{
    check_run_application(); // boot app before any peripheral initialization
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);

    bool should_timeout = true;

    timeout_timer_init(F_CPU, TIMEOUT_S * 1000);

    uart_init(115200);
    uart_puts("ASEBA bootloader started\n");

    aseba_can_init();
    aseba_send_descr();

    aseba_bootloader_context_t ctx;
    aseba_bootloader_context_init(&ctx);

    uint16_t msg[4];
    while (1) {
        int msg_size = aseba_can_receive(msg);
        if (msg_size >= 2) {
            // uart_puts("ASEBA frame received.\n");
            if (msg[1] == ASEBA_ID) {
                // uart_puts("ASEBA ID match\n");
                aseba_cmd_exec(&ctx, msg[0], &msg[2], msg_size-2);
                should_timeout = false;
            } else {
                // uart_puts("(not my id)\n");
            }
        }

        if (should_timeout && timeout_reached()) {
            break;
        }
    }

    reboot_to_application();

    return 0;
}
