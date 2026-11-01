/* Exercise the actual EEPROM load/save path, including checksums and backups. */
#include <assert.h>
#include <stdio.h>
#include "../src/game/save_file.c"

struct SaveBuffer gSaveBuffer;
s8 gEepromProbe = 1;
OSMesgQueue gSIEventMesgQueue;
static unsigned char eeprom[EEPROM_SIZE];
static int writes;
s32 osEepromLongRead(OSMesgQueue *queue, u8 block, u8 *data, int size) {
    (void)queue;
    assert(block * 8 + size <= EEPROM_SIZE);
    memcpy(data, eeprom + block * 8, size);
    return 0;
}
s32 osEepromLongWrite(OSMesgQueue *queue, u8 block, u8 *data, int size) {
    (void)queue;
    assert(block * 8 + size <= EEPROM_SIZE);
    memcpy(eeprom + block * 8, data, size);
    ++writes;
    return 0;
}
int main(void) {
    unsigned char original_abc[sizeof(gSaveBuffer.files[0]) * 3];
    int file, slot, writes_after_first_load;
    memset(&gSaveBuffer, 0, sizeof(gSaveBuffer));
    for (slot = 0; slot < 2; ++slot) {
        add_save_block_signature(&gSaveBuffer.menuData[slot], sizeof(gSaveBuffer.menuData[slot]), MENU_DATA_MAGIC);
    }
    for (file = 0; file < NUM_SAVE_FILES; ++file) {
        for (slot = 0; slot < 2; ++slot) {
            struct SaveFile *save = &gSaveBuffer.files[file][slot];
            save->flags = SAVE_FLAG_FILE_EXISTS;
            save->courseStars[0] = 1 << file;
            save->courseCoinScores[0] = 150 + file;
            add_save_block_signature(save, sizeof(*save), SAVE_FILE_MAGIC);
        }
    }
    memcpy(original_abc, gSaveBuffer.files, sizeof(original_abc));
    memcpy(eeprom, &gSaveBuffer, sizeof(eeprom));
    save_file_load_all();
    assert(!memcmp(original_abc, gSaveBuffer.files, sizeof(original_abc)));
    assert(!memcmp(original_abc, eeprom + offsetof(struct SaveBuffer, files), sizeof(original_abc)));
    assert(save_file_get_total_star_count(SAVE_FILE_D, COURSE_MIN - 1, COURSE_MAX - 1) == 120);
    assert(gSaveBuffer.files[SAVE_FILE_D][0].courseCoinScores[0] == 153);
    assert(gSaveBuffer.files[SAVE_FILE_D][0].courseStars[COURSE_PSS - 1] == 3);
    for (file = COURSE_MIN; file <= COURSE_STAGES_MAX; ++file) {
        assert(gSaveBuffer.files[SAVE_FILE_D][0].courseStars[file] & 0x80);
    }
    assert((gSaveBuffer.files[SAVE_FILE_D][0].flags & (SAVE_FLAG_HAVE_WING_CAP | SAVE_FLAG_HAVE_METAL_CAP | SAVE_FLAG_HAVE_VANISH_CAP)) == 14);
    for (slot = 0; slot < 2; ++slot) {
        assert(verify_save_block_signature(&gSaveBuffer.files[SAVE_FILE_D][slot], sizeof(struct SaveFile), SAVE_FILE_MAGIC));
    }
    assert(!memcmp(&gSaveBuffer.files[SAVE_FILE_D][0], &gSaveBuffer.files[SAVE_FILE_D][1], sizeof(struct SaveFile)));
    writes_after_first_load = writes;
    memset(&gSaveBuffer, 0, sizeof(gSaveBuffer));
    save_file_load_all();
    assert(save_file_get_total_star_count(SAVE_FILE_D, 0, COURSE_MAX - 1) == 120);
    assert(writes == writes_after_first_load); // completed save does not rewrite every boot
    puts("save D: 120 stars, caps, retained records, ABC unchanged, backup/checksum and reload PASS");
    return 0;
}
