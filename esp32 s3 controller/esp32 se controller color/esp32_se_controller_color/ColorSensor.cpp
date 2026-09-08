
#include "ColorSensor.h"

#include "Adafruit_AS7341.h"
#include <math.h>
#include <string.h>

namespace {
Adafruit_AS7341 as7341;
bool sensorColorOk = false;

const float cieX[8] = {0.0776, 0.3481, 0.0956, 0.0291, 0.5121, 1.0263, 0.6424, 0.0468};
const float cieY[8] = {0.0022, 0.0298, 0.1390, 0.6082, 1.0000, 0.7570, 0.2650, 0.0170};
const float cieZ[8] = {0.3713, 1.7826, 0.8130, 0.1117, 0.0057, 0.0011, 0.0000, 0.0000};
struct ColorRef {
  const char* name;
  float ch[8];
};
constexpr float SCHWARZ_WEISS_GRENZE = 8000.0f;

const ColorRef colorRefs[] = {
  // 1 cm
  {"Schwarz",   {103, 410, 251, 478, 733, 713, 588, 271}},
  {"Weiß",      {727, 3614, 1591, 4510, 6659, 5797, 4293, 2029}},
  {"Rot",       {215, 615, 383, 716, 1235, 2429, 3248, 1681}},
  {"Gelb",      {326, 626, 697, 2832, 5804, 5537, 4086, 1814}},
  {"Grün",      {136, 452, 461, 1639, 1328, 849, 662, 329}},
  {"Dunkelblau",{283, 2058, 780, 1155, 999, 860, 731, 380}},
  {"Orange",    {273, 755, 465, 1054, 3075, 4327, 3480, 1500}},
  {"Hellblau",  {438, 2768, 1256, 3043, 2750, 1714, 1246, 627}},
  {"Magenta",   {241, 992, 402, 636, 948, 1603, 2459, 1376}},
  

  // 1,5 cm
  {"Schwarz",   {94, 365, 234, 423, 660, 659, 555, 257}},
  {"Weiß",      {468, 2265, 1033, 2840, 4216, 3734, 2817, 1330}},
  {"Rot",       {143, 428, 290, 497, 868, 1564, 2003, 1012}},
  {"Gelb",      {216, 464, 482, 1739, 3523, 3425, 2594, 1168}},
  {"Grün",      {108, 370, 349, 1072, 967, 710, 579, 285}},
  {"Dunkelblau",{187, 1222, 505, 751, 757, 707, 618, 319}},
  {"Magenta",   {178, 693, 322, 497, 762, 1198, 1735, 970}},
  {"Hellblau",  {286, 1649, 792, 1841, 1746, 1209, 935, 466}},
  {"Orange",    {221, 616, 394, 846, 2398, 3360, 2729, 1194}},

  // 2 cm
  {"Schwarz",   {85, 308, 217, 354, 574, 602, 529, 264}},
  {"Weiß",      {341, 1512, 743, 1922, 2869, 2637, 2087, 1086}},
  {"Rot",       {132, 377, 273, 437, 742, 1217, 1502, 838}},
  {"Gelb",      {174, 397, 388, 1177, 2334, 2344, 1874, 940}},
  {"Grün",      {99, 328, 300, 776, 788, 663, 572, 297}},
  {"Dunkelblau",{156, 879, 406, 598, 671, 671, 600, 320}},
  {"Magenta",   {145, 530, 282, 430, 672, 980, 1322, 741}},
  {"Hellblau",  {219, 1175, 595, 1330, 1329, 1006, 821, 411}},
  {"Orange",    {151, 442, 301, 579, 1513, 2061, 1734, 781}},  
};
const int NUM_COLORS = sizeof(colorRefs) / sizeof(colorRefs[0]);

const char* klassifiziereFarbe(float spec[8]) {
  float summeMessung = 0.0f;

  for (int i = 0; i < 8; i++) {
    summeMessung += spec[i];
  }

  if (summeMessung <= 0.0f) {
    return "Unbekannt";
  }

  int bestIdx = 0;
  float bestDist = INFINITY;

  for (int c = 0; c < NUM_COLORS; c++) {
    float summeReferenz = 0.0f;

    for (int i = 0; i < 8; i++) {
      summeReferenz += colorRefs[c].ch[i];
    }

    float dist = 0.0f;

    for (int i = 0; i < 8; i++) {
      float messAnteil = spec[i] / summeMessung;
      float refAnteil = colorRefs[c].ch[i] / summeReferenz;
      float diff = messAnteil - refAnteil;
      dist += diff * diff;
    }

    if (dist < bestDist) {
      bestDist = dist;
      bestIdx = c;
    }
  }

  const char* name = colorRefs[bestIdx].name;

  if (strcmp(name, "Schwarz") == 0 || strcmp(name, "Weiß") == 0) {
    return summeMessung < SCHWARZ_WEISS_GRENZE ? "Schwarz" : "Weiß";
  }

  return name;
}
volatile int sR = 0;
volatile int sG = 0;
volatile int sB = 0;
char sColorName[32] = "Unbekannt";
volatile bool sColorNew = false;

String obtenerNombreColor(int r, int g, int b, float intensidadTotal) {
  if (intensidadTotal < 15.0) return "Schwarz";

  float rf = r / 255.0;
  float gf = g / 255.0;
  float bf = b / 255.0;

  float cmax = max(rf, max(gf, bf));
  float cmin = min(rf, min(gf, bf));
  float delta = cmax - cmin;

  float s = (cmax == 0) ? 0 : (delta / cmax);

  if (s < 0.15) return "Weiß";

  float h = 0;

  if (delta > 0) {
    if (cmax == rf) {
      h = 60.0 * ((gf - bf) / delta);
      if (h < 0) h += 360.0;
    } else if (cmax == gf) {
      h = 60.0 * (((bf - rf) / delta) + 2.0);
    } else if (cmax == bf) {
      h = 60.0 * (((rf - gf) / delta) + 4.0);
    }
  }

  if (h >= 0 && h < 12) return "Rot";
  if (h >= 12 && h < 30) return "Orange";
  if (h >= 30 && h < 75) return "Gelb";
  if (h >= 75 && h < 160) return "Grün";
  if (h >= 160 && h < 200) return "Cyan";
  if (h >= 200 && h < 260) return "Blau";
  if (h >= 260 && h < 320) return "Violett";
  if (h >= 320 && h < 350) return "Rosa";
  if (h >= 350 && h <= 360) return "Rot";

  return "Unbekannt";
}
}

namespace ColorSensor {
void begin() {
  if (as7341.begin()) {
    as7341.setATIME(100);
    as7341.setASTEP(999);
    as7341.setGain(AS7341_GAIN_16X);
    as7341.setLEDCurrent(10);
    as7341.enableLED(true);
    sensorColorOk = true;
  }
}

bool isOk() {
  return sensorColorOk;
}

void update() {
  uint16_t ch[12];
  if (!sensorColorOk) return;
  if (!as7341.readAllChannels(ch)) return;
  Serial.printf("RAW: %u %u %u %u %u %u %u %u\n", ch[0], ch[1], ch[2], ch[3], ch[6], ch[7], ch[8], ch[9]);


  float spec[8] = {(float)ch[0], (float)ch[1], (float)ch[2], (float)ch[3],
                   (float)ch[6], (float)ch[7], (float)ch[8], (float)ch[9]};
  float intTotal = 0;
  float X = 0;
  float Y = 0;
  float Z = 0;
  for (int i = 0; i < 8; i++) {
    intTotal += spec[i];
    X += spec[i] * cieX[i];
    Y += spec[i] * cieY[i];
    Z += spec[i] * cieZ[i];
  }

  float R = 3.2406 * X - 1.5372 * Y - 0.4986 * Z;
  float G = -0.9689 * X + 1.8758 * Y + 0.0415 * Z;
  float B = 0.0557 * X - 0.2040 * Y + 1.0570 * Z;

  if (R < 0) R = 0;
  if (G < 0) G = 0;
  if (B < 0) B = 0;

  float maxRGB = max(R, max(G, B));
  if (maxRGB > 0) {
    R /= maxRGB;
    G /= maxRGB;
    B /= maxRGB;
  }

  R = pow(R, 1.0 / 2.2);
  G = pow(G, 1.0 / 2.2);
  B = pow(B, 1.0 / 2.2);
  int r = (int)(R * 255.0);
  int g = (int)(G * 255.0);
  int b = (int)(B * 255.0);

  sR = r;
  sG = g;
  sB = b;
  const char* colorName = klassifiziereFarbe(spec);
  Serial.print("Farbe: ");
  Serial.println(colorName);
  strncpy(sColorName, colorName, sizeof(sColorName) - 1);
  sColorName[sizeof(sColorName) - 1] = '\0';
  sColorNew = true;
}

bool consumeNewReading(int& r, int& g, int& b, char* colorName, size_t colorNameSize) {
  if (!sColorNew) return false;

  sColorNew = false;
  r = sR;
  g = sG;
  b = sB;
  if (colorName != nullptr && colorNameSize > 0) {
    strncpy(colorName, sColorName, colorNameSize - 1);
    colorName[colorNameSize - 1] = '\0';
  }
  return true;
}

int red() {
  return sR;
}

int green() {
  return sG;
}

int blue() {
  return sB;
}

bool isColor(const char* protocolColor) {
  const char* colorMap[][2] = {
      {"RED", "Rot"},       {"GREEN", "Grün"},     {"BLUE", "Blau"},
      {"YELLOW", "Gelb"},   {"WHITE", "Weiß"},     {"BLACK", "Schwarz"},
      {"ORANGE", "Orange"}, {"CYAN", "Cyan"},      {"VIOLET", "Violett"},
      {"PINK", "Rosa"}};

  for (auto& m : colorMap) {
    if (strcmp(protocolColor, m[0]) == 0) {
      return strcmp(sColorName, m[1]) == 0;
    }
  }
  return false;
}

const char* name() {
  return sColorName;
}

}  // namespace ColorSensor
