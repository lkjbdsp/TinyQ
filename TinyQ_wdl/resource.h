#define PLUG_MFR "lkjb"
#define PLUG_NAME "TinyQ"

#define PLUG_CLASS_NAME TinyQ

#define BUNDLE_MFR "lkjb"
#define BUNDLE_NAME "TinyQ"

#define PLUG_ENTRY TinyQ_Entry
#define PLUG_VIEW_ENTRY TinyQ_ViewEntry

#define PLUG_ENTRY_STR "TinyQ_Entry"
#define PLUG_VIEW_ENTRY_STR "TinyQ_ViewEntry"

#define VIEW_CLASS TinyQ_View
#define VIEW_CLASS_STR "TinyQ_View"

// Format        0xMAJR.MN.BG - in HEX! so version 10.1.5 would be 0x000A0105
#define PLUG_VER 0x00010600
#define VST3_VER_STR "2.0.0"

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes. At least one capital letter
#define PLUG_UNIQUE_ID 'TinQ'
// make sure this is not the same as BUNDLE_MFR
#define PLUG_MFR_ID 'Lkjb'

// ProTools stuff
#if (defined(AAX_API) || defined(RTAS_API)) && !defined(_PIDS_)
  #define _PIDS_
  const int PLUG_TYPE_IDS[2] = {'GRN1', 'GRN2'};
  //const int PLUG_TYPE_IDS_AS[2] = {'GRA1', 'GRA2'}; // AudioSuite
#endif

#define PLUG_MFR_PT "lkjb\nlkjb"
#define PLUG_NAME_PT "TinyQ"
#define PLUG_TYPE_PT "EQ"
#define PLUG_DOES_AUDIOSUITE 0

/* PLUG_TYPE_PT can be "None", "EQ", "Dynamics", "PitchShift", "Reverb", "Delay", "Modulation", 
"Harmonic" "NoiseReduction" "Dither" "SoundField" "Effect" 
instrument determined by PLUG _IS _INST
*/

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0

// if this is 0 RTAS can't get tempo info
#define PLUG_DOES_MIDI 0

#define PLUG_DOES_STATE_CHUNKS 1

// Unique IDs for each image resource.
#define BACKGROUND_1_0_ID       101
#define BACKGROUND_1_5_ID       102
#define BACKGROUND_2_0_ID       103
#define KNOB_BLUE_1_0_ID        105
#define KNOB_GREEN_1_0_ID       106
#define KNOB_GREY_1_0_ID        107
#define KNOB_RED_1_0_ID         108
#define KNOB_YELLOW_1_0_ID      109
#define KNOB_BLUE_1_5_ID        110
#define KNOB_GREEN_1_5_ID       111
#define KNOB_GREY_1_5_ID        112
#define KNOB_RED_1_5_ID         113
#define KNOB_YELLOW_1_5_ID      114
#define KNOB_BLUE_2_0_ID        115
#define KNOB_GREEN_2_0_ID       116
#define KNOB_GREY_2_0_ID        117
#define KNOB_RED_2_0_ID         118
#define KNOB_YELLOW_2_0_ID      119
#define KNOB_HPORDER_1_0_ID     120
#define KNOB_HPORDER_1_5_ID     121
#define KNOB_HPORDER_2_0_ID     122
#define KNOB_LPORDER_1_0_ID     123
#define KNOB_LPORDER_1_5_ID     124
#define KNOB_LPORDER_2_0_ID     125
#define BUTTON_LOW_1_0_ID       126
#define BUTTON_LOW_1_5_ID       127
#define BUTTON_LOW_2_0_ID       128
#define BUTTON_LOMID_1_0_ID     129
#define BUTTON_LOMID_1_5_ID     130
#define BUTTON_LOMID_2_0_ID     131
#define BUTTON_HIMID_1_0_ID     132
#define BUTTON_HIMID_1_5_ID     133
#define BUTTON_HIMID_2_0_ID     134
#define BUTTON_HIGH_1_0_ID      135
#define BUTTON_HIGH_1_5_ID      136
#define BUTTON_HIGH_2_0_ID      137
#define BUTTON_HP_1_0_ID        138
#define BUTTON_HP_1_5_ID        139
#define BUTTON_HP_2_0_ID        140
#define BUTTON_LP_1_0_ID        141
#define BUTTON_LP_1_5_ID        142
#define BUTTON_LP_2_0_ID        143
#define BUTTON_SIZE_1_0_ID      144
#define BUTTON_SIZE_1_5_ID      145
#define BUTTON_SIZE_2_0_ID      146
#define EDITOR_FONT_1_0_ID      147
#define EDITOR_FONT_1_5_ID      148
#define EDITOR_FONT_2_0_ID      149
#define BUTTON_SHELF_1_0_ID     150
#define BUTTON_SHELF_1_5_ID     151
#define BUTTON_SHELF_2_0_ID     152


// Image resource locations for this plug.
#define BACKGROUND_1_0_FN    "resources/gui/Background_1-0.png"
#define BACKGROUND_1_5_FN    "resources/gui/Background_1-5.png"
#define BACKGROUND_2_0_FN    "resources/gui/Background_2-0.png"
#define KNOB_BLUE_1_0_FN     "resources/gui/blue.png"
#define KNOB_GREEN_1_0_FN    "resources/gui/green.png"
#define KNOB_GREY_1_0_FN     "resources/gui/grey.png"
#define KNOB_RED_1_0_FN      "resources/gui/red.png"
#define KNOB_YELLOW_1_0_FN   "resources/gui/yellow.png"
#define KNOB_BLUE_1_5_FN     "resources/gui/blue_1-5.png"
#define KNOB_GREEN_1_5_FN    "resources/gui/green_1-5.png"
#define KNOB_GREY_1_5_FN     "resources/gui/grey_1-5.png"
#define KNOB_RED_1_5_FN      "resources/gui/red_1-5.png"
#define KNOB_YELLOW_1_5_FN   "resources/gui/yellow_1-5.png"
#define KNOB_BLUE_2_0_FN     "resources/gui/blue_2-0.png"
#define KNOB_GREEN_2_0_FN    "resources/gui/green_2-0.png"
#define KNOB_GREY_2_0_FN     "resources/gui/grey_2-0.png"
#define KNOB_RED_2_0_FN      "resources/gui/red_2-0.png"
#define KNOB_YELLOW_2_0_FN   "resources/gui/yellow_2-0.png"
#define KNOB_HPORDER_1_0_FN  "resources/gui/locutorder.png"
#define KNOB_HPORDER_1_5_FN  "resources/gui/locutorder_1-5.png"
#define KNOB_HPORDER_2_0_FN  "resources/gui/locutorder_2-0.png"
#define KNOB_LPORDER_1_0_FN  "resources/gui/hicutorder.png"
#define KNOB_LPORDER_1_5_FN  "resources/gui/hicutorder_1-5.png"
#define KNOB_LPORDER_2_0_FN  "resources/gui/hicutorder_2-0.png"
#define BUTTON_LOW_1_0_FN    "resources/gui/lowbutton.png"
#define BUTTON_LOW_1_5_FN    "resources/gui/lowbutton_1-5.png"
#define BUTTON_LOW_2_0_FN    "resources/gui/lowbutton_2-0.png"
#define BUTTON_LOMID_1_0_FN  "resources/gui/lomidbutton.png"
#define BUTTON_LOMID_1_5_FN  "resources/gui/lomidbutton_1-5.png"
#define BUTTON_LOMID_2_0_FN  "resources/gui/lomidbutton_2-0.png"
#define BUTTON_HIMID_1_0_FN  "resources/gui/himidbutton.png"
#define BUTTON_HIMID_1_5_FN  "resources/gui/himidbutton_1-5.png"
#define BUTTON_HIMID_2_0_FN  "resources/gui/himidbutton_2-0.png"
#define BUTTON_HIGH_1_0_FN   "resources/gui/highbutton.png"
#define BUTTON_HIGH_1_5_FN   "resources/gui/highbutton_1-5.png"
#define BUTTON_HIGH_2_0_FN   "resources/gui/highbutton_2-0.png"
#define BUTTON_HP_1_0_FN     "resources/gui/hpbutton.png"
#define BUTTON_HP_1_5_FN     "resources/gui/hpbutton_1-5.png"
#define BUTTON_HP_2_0_FN     "resources/gui/hpbutton_2-0.png"
#define BUTTON_LP_1_0_FN     "resources/gui/lpbutton.png"
#define BUTTON_LP_1_5_FN     "resources/gui/lpbutton_1-5.png"
#define BUTTON_LP_2_0_FN     "resources/gui/lpbutton_2-0.png"
#define BUTTON_SIZE_1_0_FN   "resources/gui/sizebutton.png"
#define BUTTON_SIZE_1_5_FN   "resources/gui/sizebutton_1-5.png"
#define BUTTON_SIZE_2_0_FN   "resources/gui/sizebutton_2-0.png"
#define EDITOR_FONT_1_0_FN   "resources/gui/font.png"
#define EDITOR_FONT_1_5_FN   "resources/gui/font_1-5.png"
#define EDITOR_FONT_2_0_FN   "resources/gui/font_2-0.png"
#define BUTTON_SHELF_1_0_FN  "resources/gui/shelfswitch.png"
#define BUTTON_SHELF_1_5_FN  "resources/gui/shelfswitch_1-5.png"
#define BUTTON_SHELF_2_0_FN  "resources/gui/shelfswitch_2-0.png"


// GUI default dimensions
#define GUI_WIDTH 204
#define GUI_HEIGHT 379

// on MSVC, you must define SA_API in the resource editor preprocessor macros as well as the c++ ones
#if defined(SA_API) && !defined(OS_IOS)
#include "app_wrapper/app_resource.h"
#endif

// vst3 stuff
#define MFR_URL "www.kvraudio.com/developer/lkjb"
#define MFR_EMAIL "lkjbdsp@gmail.com"
#define EFFECT_TYPE_VST3 "Fx|EQ"

/* "Fx|Analyzer"", "Fx|Delay", "Fx|Distortion", "Fx|Dynamics", "Fx|EQ", "Fx|Filter",
"Fx", "Fx|Instrument", "Fx|InstrumentExternal", "Fx|Spatial", "Fx|Generator",
"Fx|Mastering", "Fx|Modulation", "Fx|PitchShift", "Fx|Restoration", "Fx|Reverb",
"Fx|Surround", "Fx|Tools", "Instrument", "Instrument|Drum", "Instrument|Sampler",
"Instrument|Synth", "Instrument|Synth|Sampler", "Instrument|External", "Spatial",
"Spatial|Fx", "OnlyRT", "OnlyOfflineProcess", "Mono", "Stereo",
"Surround"
*/
