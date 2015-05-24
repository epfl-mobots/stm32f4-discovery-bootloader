#include <libopencm3/stm32/flash.h>
#include "aseba_flash.h"

uint16_t *aseba_page_address(int aseba_page, int word_offset)
{
    return ((uint16_t*)ASEBA_FIRST_PAGE_ADDR) + (aseba_page*ASEBA_PAGE_SIZE_IN_WORDS + word_offset);
}

// stm32f4 flash sector of an aseba page
// returns -1 if the page is not at beginning of the sector
int aseba_page_sector(int aseba_page)
{
    if (aseba_page % ASEBA_PAGES_PER_SECTOR == 0) { // first page in sector
        int sector_number = ASEBA_PAGE_FIRST_SECTOR + aseba_page / ASEBA_PAGES_PER_SECTOR;
        return sector_number;
    } else {
        return -1;
    }
}

uint16_t aseba_flash_read_word(int aseba_page, int word_offset)
{
    return *aseba_page_address(aseba_page, word_offset);
}

void aseba_flash_erase_page(int aseba_page)
{
    flash_unlock();
    int sector = aseba_page_sector(aseba_page);
    if (sector != -1) { // first page in sector
        flash_erase_sector(sector, FLASH_CR_PROGRAM_X16);
    }
}

void aseba_flash_write_page(int aseba_page, uint16_t *page_buffer)
{
    int i;
    for (i = 0; i < ASEBA_PAGE_SIZE_IN_WORDS; i++) {
        flash_program_half_word((uint32_t)aseba_page_address(aseba_page, i), page_buffer[i]);
    }
    flash_lock();
}
