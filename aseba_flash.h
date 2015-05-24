#include <stdint.h>
/* STM32F4 flash memory layout:
 * Sector 0     0x0800 0000 - 0x0800 3FFF   16 Kbytes
 * Sector 1     0x0800 4000 - 0x0800 7FFF   16 Kbytes
 * Sector 2     0x0800 8000 - 0x0800 BFFF   16 Kbytes
 * Sector 3     0x0800 C000 - 0x0800 FFFF   16 Kbytes
 * Sector 4     0x0801 0000 - 0x0801 FFFF   64 Kbytes
 * Sector 5     0x0802 0000 - 0x0803 FFFF   128 Kbytes
 * Sector 6     0x0804 0000 - 0x0805 FFFF   128 Kbytes
 * ...
 * Sector 11    0x080E 0000 - 0x080F FFFF   128 Kbytes
 */

/*
 * Flash as advertised to ASEBA:
 * - all page sizes are 16kbytes
 * - only the large (128k) sectors are available for the application
 * -> there are 8 aseba pages in a flash sector
 * Note: flash erasing only works if the aseba pages are written in ascending order
 */

#define ASEBA_AVAILABLE_PAGES (7*8) // 7 x 128k sectors = 7 x 8 aseba sectors
#define ASEBA_FIRST_PAGE_ADDR (void*)0x08020000
#define ASEBA_PAGE_SIZE (16*1024) // bytes
#define ASEBA_PAGE_SIZE_IN_WORDS (ASEBA_PAGE_SIZE/2)
#define ASEBA_PAGES_PER_SECTOR 8
#define ASEBA_PAGE_FIRST_SECTOR 5

uint16_t aseba_flash_read_word(int aseba_page, int word_offset);
void aseba_flash_erase_page(int aseba_page);
void aseba_flash_write_page(int aseba_page, uint16_t *page_buffer);
