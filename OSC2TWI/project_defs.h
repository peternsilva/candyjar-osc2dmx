/**
 *  PROJECT: DR JOHN'S CANDY JAR
 *
 *
 *
 */

/**
 *	Define each fixture type and it's DMX operating modes with a number
 */
#define FIXTURE_ADJ_38B_3CH  1
#define FIXTURE_ADJ_38B_6CH  2
#define FIXTURE_ADJ_38B_7CH  3
#define FIXTURE_ADJ_MEGA_PAR_1CH  4
#define FIXTURE_ADJ_MEGA_PAR_2CH  5
#define FIXTURE_ADJ_MEGA_PAR_3CH  6
#define FIXTURE_ADJ_MEGA_PAR_4CH  7
#define FIXTURE_ADJ_MEGA_PAR_5CH  8
#define FIXTURE_ADJ_MEGA_PAR_6CH  9
#define FIXTURE_ADJ_MEGA_PAR_7CH  10
#define FIXTURE_ADJ_H2O_3CH  11
#define FIXTURE_RGB_STRIP_3CH  12
#define FIXTURE_MAGICAL_BALL_6CH  13
#define FIXTURE_SHARP_BEAMER_7CH  14
#define FIXTURE_SWARM_FX_18CH  15

/**
 *	Define the different lighting areas that fixtures can live in
 */
#define AREA_GLOBAL 0

#define NUM_LIGHTING_AREAS 7

#define AREA_STANDUP_KIT 1
#define AREA_DRUM_RISERS 2
#define AREA_DRUMS_OVERHEAD 3
#define AREA_HALFWALL 4
#define AREA_DJ_BOOTH 5
#define AREA_AUX1 6
#define AREA_AUX2 7

/**
 *	Define the different parameters that are available to be controlled per lighting area
 */
#define NUM_AREA_PARAMS 3
#define AREA_PARAM_HUE 0
#define AREA_PARAM_SATURATION 1
#define AREA_PARAM_BRIGHTNESS 2


/**
 *	Define the fixture topology of the project
 *	Define the number of fixtures, which types, located where, with what DMX address
 *	E.g., fixtures[fixtureIndex][fieldId] = { area, fixtureType, dmxStart }
 *
 *	Edit this if you change fixtures, or move them around
 */
#define NUM_FIXTURES 15
unsigned int fixtures[NUM_FIXTURES][3] =
{
    { AREA_STANDUP_KIT, FIXTURE_SWARM_FX_18CH, 1 },
    { AREA_STANDUP_KIT, FIXTURE_ADJ_H2O_3CH, 19 },

    { AREA_DRUM_RISERS, FIXTURE_RGB_STRIP_3CH, 22 },
    { AREA_DRUM_RISERS, FIXTURE_RGB_STRIP_3CH, 25 },
    { AREA_DRUM_RISERS, FIXTURE_RGB_STRIP_3CH, 28 },

    { AREA_DRUMS_OVERHEAD, FIXTURE_ADJ_38B_7CH, 31 },
    { AREA_DRUMS_OVERHEAD, FIXTURE_ADJ_38B_7CH, 38 },
    { AREA_DRUMS_OVERHEAD, FIXTURE_ADJ_38B_7CH, 45 },

    { AREA_HALFWALL, FIXTURE_ADJ_38B_7CH, 52 },
    { AREA_HALFWALL, FIXTURE_ADJ_38B_7CH, 59 },
    { AREA_HALFWALL, FIXTURE_ADJ_38B_7CH, 66 },

    { AREA_DJ_BOOTH, FIXTURE_ADJ_MEGA_PAR_7CH, 73 },

    // Lost MEGA PARs
    { AREA_DJ_BOOTH, FIXTURE_ADJ_MEGA_PAR_7CH, 80 },
    { AREA_DJ_BOOTH, FIXTURE_ADJ_MEGA_PAR_7CH, 87 },
    { AREA_DJ_BOOTH, FIXTURE_ADJ_MEGA_PAR_7CH, 94 }
};



/**
 *	Define the total number of DMX addresses this project uses
 */
#define MAX_DMX_CHANNELS 40
