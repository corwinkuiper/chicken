#include <stdint.h>
#include <tonc.h>

// graphics
#include "chicken.h"
#include "map.h"

#define SUBPIXELBITS 8

int32_t COLLISION_TILE(int32_t tileID) {
    int32_t maskedID = tileID & 0b0000001111111111;
    return maskedID == 0 || maskedID == 4;
}

struct character {
    OBJ_ATTR* obj;
    int32_t x;
    int32_t y;
    int32_t xVelocity;
    int32_t yVelocity;
};

struct position {
    int32_t x;
    int32_t y;
};

OBJ_ATTR obj_buffer[128];

int32_t frameRanger(int32_t count, int32_t start, int32_t end, int32_t fDelay) {
    return ((count / fDelay) % (end + 1 - start)) + start;
}

int main(void) {

    // load map
    memcpy32(pal_bg_mem, mapPal, mapPalLen / 4);
    memcpy16(&tile_mem[0], mapTiles, mapTilesLen / 2);
    memcpy16(&se_mem[30], mapMap, mapMapLen / 2);

    // display mode
    REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_4BPP | BG_REG_32x32;
    REG_DISPCNT = DCNT_OBJ | DCNT_BG0;

    // vblank interrupt
    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    // initialise objects
    oam_init(obj_buffer, 128);
    // copy sprite palette
    memcpy32(pal_obj_mem, chickenPal, chickenPalLen / 4);
    memcpy32(&tile_mem_obj[0], chickenTiles, chickenTilesLen / 2);

    struct character chicken;

    #define chickenPalette 0

    chicken.obj = &obj_buffer[0];
    obj_set_attr(chicken.obj, ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(chickenPalette) | 0);
    chicken.x = ((6 << 3) << SUBPIXELBITS);
    chicken.y = (((7 << 3) - 4) << SUBPIXELBITS);
    chicken.xVelocity = 0;
    chicken.yVelocity = 0;
    

    int32_t frameCounter = 0;

    while (1) {
        ++frameCounter;

        VBlankIntrWait();
        key_poll();

        // ------ set velocity based on inputs --------------------------------

        chicken.xVelocity -= chicken.xVelocity >> 3;
        if (key_tri_horz() == 1) {
            chicken.obj->attr1 = ATTR1_SIZE_8;
        } else if (key_tri_horz() == -1) {
            chicken.obj->attr1 = ATTR1_SIZE_8 | ATTR1_HFLIP;
        }
        if (key_tri_horz()) {
            chicken.xVelocity = key_tri_horz() << 16;
        }

        // ------ collision detection -----------------------------------------

        int32_t newChickenX = chicken.x + (chicken.xVelocity >> SUBPIXELBITS);
        int32_t newChickenY = chicken.y + (chicken.yVelocity >> SUBPIXELBITS);

        int32_t tileX = (newChickenX >> SUBPIXELBITS) >> 3;
        int32_t tileY = (chicken.y >> SUBPIXELBITS) >> 3;

        int32_t leftTile, rightTile, topTile, bottomTile;
        leftTile = ((newChickenX >> SUBPIXELBITS) - 4) >> 3;
        rightTile = ((newChickenX >> SUBPIXELBITS) + 4) >> 3;

        topTile = ((newChickenY >> SUBPIXELBITS) - 4) >> 3;
        bottomTile = ((newChickenY >> SUBPIXELBITS) + 4) >> 3;

        // bit shift up by 5 is a multiplication by 32.

        if ((chicken.xVelocity < 0) && (COLLISION_TILE(mapMap[(tileY * 32) + leftTile]))) {
            newChickenX = (((leftTile + 1) * 8 + 4) << SUBPIXELBITS);
            chicken.xVelocity = 0;
        } else if ((chicken.xVelocity > 0) && (COLLISION_TILE(mapMap[(tileY * 32) + rightTile]))) {
            newChickenX = ((rightTile * 8 - 4) << SUBPIXELBITS);
            chicken.xVelocity = 0;
        }

        int32_t isFlappingAnimation = 0;

        if ((chicken.yVelocity < 0) && (COLLISION_TILE(mapMap[(topTile * 32) + tileX]))) {
            newChickenY = (((topTile + 1) * 8 + 4) << SUBPIXELBITS) + 4;
            chicken.yVelocity = 0;
        }
        if ((chicken.yVelocity > 0) && (COLLISION_TILE(mapMap[(bottomTile * 32) + tileX]))) {
            chicken.yVelocity = 0;
            newChickenY = ((bottomTile * 8 - 4) << SUBPIXELBITS);
        }
        if (!COLLISION_TILE(mapMap[(bottomTile * 32) + tileX])) {
            // apply downward gravity
            if (chicken.yVelocity < 0) {
                isFlappingAnimation = 1;
                chicken.yVelocity += 1 << (SUBPIXELBITS + 4);
            } else {
                isFlappingAnimation = 2;
                chicken.yVelocity += (1 << (SUBPIXELBITS + 4)) / 3;
            }
        }

        if ((!isFlappingAnimation) && (key_hit(KEY_A))) {
            chicken.yVelocity = -(1 << (SUBPIXELBITS + 9));
        }

        chicken.x = newChickenX;
        chicken.y = newChickenY;

        if (chicken.x < (4 << SUBPIXELBITS)) {
            chicken.x = (4 << SUBPIXELBITS);
        } else if ((chicken.x >> SUBPIXELBITS) > (SCREEN_WIDTH - 4)) {
            chicken.x = (SCREEN_WIDTH - 4) << SUBPIXELBITS;
        }

        // ------ animation ---------------------------------------------------

        int32_t isMoving = !((chicken.xVelocity < (1 << (SUBPIXELBITS + 5))) && (chicken.xVelocity > -(1 << (SUBPIXELBITS + 5))));

        if (isFlappingAnimation) {
            if (isFlappingAnimation == 2) {
                chicken.obj->attr2 = ATTR2_BUILD(frameRanger(frameCounter, 4, 5, 5), chickenPalette, 0);
            }
        } else if (isMoving) {
            chicken.obj->attr2 = ATTR2_BUILD(frameRanger(frameCounter, 1, 3, 10), chickenPalette, 0);
        } else {
            chicken.obj->attr2 = ATTR2_BUILD(0, chickenPalette, 0);
        }

        // ------ set position and swap in objects ----------------------------

        obj_set_pos(chicken.obj, (chicken.x >> SUBPIXELBITS) - 4, (chicken.y >> SUBPIXELBITS) - 4);

        oam_copy(oam_mem, obj_buffer, 1);
    }

    return 0;
}