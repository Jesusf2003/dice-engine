#ifndef __RADIANT_CONFIG_H__
#define __RADIANT_CONFIG_H__

typedef struct radiantdefs_t
{
    const char *title = "Radiant Editor";
    const char *version = "0.1.0";
    int width = 1600;
    int height = 900;
    const char *wadPath = "ninjawars";    // código temporal
} radiantdefs_s;

extern radiantdefs_t radiantdef;

#endif
