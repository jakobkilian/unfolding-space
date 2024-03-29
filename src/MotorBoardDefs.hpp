//----------------------------------------------------------------------
// ADDRESSES for Registers and I2C
//----------------------------------------------------------------------
// Adresses on the DRV2605 Register
#define STATUS 0x00
#define MODE 0x01
#define RTP_INPUT 0x02
#define LIB_SEL 0x03
#define WAV_SEQ1 0x04
#define WAV_SEQ2 0x05
#define WAV_SEQ3 0x06
#define WAV_SEQ4 0x07
#define WAV_SEQ5 0x08
#define WAV_SEQ6 0x09
#define WAV_SEQ7 0x0A
#define WAV_SEQ8 0x0B
#define GO 0x0C
#define ODT_OFFSET 0x0D
#define SPT 0x0E
#define SNT 0x0F
#define BRT 0x10
#define ATV_CON 0x11
#define ATV_MIN_IN 0x12
#define ATV_MAX_IN 0x13
#define ATV_MIN_OUT 0x14
#define ATV_MAX_OUT 0x15
#define RATED_VOLTAGE 0x16
#define OD_CLAMP 0x17
#define A_CAL_COMP 0x18
#define A_CAL_BEMF 0x19
#define FB_CON 0x1A
#define CONTRL1 0x1B
#define CONTRL2 0x1C
#define CONTRL3 0x1D
#define CONTRL4 0x1E
#define VBAT_MON 0x21
#define LRA_RESON 0x22

// I2C Adresses of TCA9548A...
#define TCA9548A_0_ADDRESS 0x70
#define TCA9548A_1_ADDRESS 0x72

//... and of DRV2605
#define DRV2605_ADDRESS 0x5A

//----------------------------------------------------------------------
// DEFINE ACTUATORS STRENGTH CURVE
//----------------------------------------------------------------------
// Vibration actuator doesn't have a linear course ( G1040003D )
const uint8_t altCurve[] = {
    0,   0,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   3,
    3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   5,
    5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   6,
    6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
    8,   8,   8,   8,   8,   8,   8,   8,   8,   8,   9,   9,   9,   9,   9,
    9,   9,   9,   10,  10,  10,  10,  10,  10,  10,  11,  11,  11,  11,  11,
    11,  12,  12,  12,  12,  12,  12,  12,  13,  13,  13,  13,  13,  14,  14,
    14,  14,  14,  15,  15,  15,  15,  15,  16,  16,  16,  16,  17,  17,  17,
    17,  17,  18,  18,  18,  18,  19,  19,  19,  19,  20,  20,  20,  21,  21,
    21,  21,  22,  22,  23,  23,  23,  24,  24,  25,  25,  25,  26,  26,  27,
    27,  28,  29,  29,  30,  30,  31,  32,  33,  33,  34,  35,  36,  37,  37,
    38,  39,  40,  41,  43,  44,  45,  46,  47,  49,  50,  51,  53,  54,  56,
    57,  59,  61,  62,  64,  66,  68,  69,  71,  73,  75,  77,  79,  81,  83,
    85,  87,  89,  91,  93,  96,  98,  100, 103, 106, 109, 112, 115, 119, 123,
    127, 132, 136, 141, 147, 153, 158, 164, 170, 177, 185, 193, 201, 211, 227,
    235, 237, 238, 239, 239, 240, 240, 241, 241, 241, 242, 242, 242, 243, 243,
    243, 243, 244, 244, 244, 244, 244, 245, 245, 245, 245, 245, 245, 246, 246,
    255};

// Motor values for the previous generation motors ( G0832012 )
//{
//    0,   20,  20,  20,  21,  21,  21,  21,  21,  21,  21,  22,  22,  22,  22,
//    22,  22,  23,  23,  23,  23,  24,  24,  24,  24,  25,  25,  25,  25,  26,
//    26,  26,  27,  27,  28,  28,  28,  29,  29,  30,  30,  31,  31,  32,  32,
//    33,  33,  34,  34,  35,  36,  36,  37,  37,  38,  39,  39,  40,  40,  41,
//    42,  42,  43,  44,  44,  45,  45,  46,  47,  47,  48,  49,  49,  50,  51,
//    51,  52,  53,  54,  54,  55,  56,  56,  57,  58,  59,  59,  60,  61,  61,
//    62,  63,  64,  64,  65,  66,  67,  67,  68,  69,  70,  70,  71,  72,  73,
//    74,  74,  75,  76,  77,  78,  78,  79,  80,  81,  82,  82,  83,  84,  85,
//    86,  87,  87,  88,  89,  90,  91,  92,  93,  93,  94,  95,  96,  97,  98,
//    99,  100, 101, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
//    113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
//    128, 129, 130, 131, 132, 133, 134, 135, 136, 138, 139, 140, 141, 142, 143,
//    144, 145, 146, 148, 149, 150, 151, 152, 153, 154, 155, 157, 158, 159, 160,
//    161, 162, 164, 165, 166, 167, 168, 169, 171, 172, 173, 174, 175, 177, 178,
//    179, 180, 181, 183, 184, 185, 186, 187, 189, 190, 191, 192, 194, 195, 196,
//    197, 198, 200, 201, 202, 203, 205, 206, 207, 208, 210, 211, 212, 213, 215,
//    216, 217, 218, 220, 221, 222, 224, 225, 227, 229, 232, 235, 239, 245,
//    254};
//
