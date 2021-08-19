/*

Thanks to wizche for the photo-viewer:
  https://github.com/wizche/flip-pics

Thanks to bitbank2 for the really fast and dithered JPEG output:
  https://github.com/bitbank2/JPEGDEC


IMPORTANT!
In your documents folder, after you've downloaded the library JPEGDEC, 
open the file : Documents\Arduino\libraries\JPEGDEC\src\JPEGDEC.h
Change line 49, to read:

#define MAX_BUFFERED_PIXELS 8192

If you don't - many pictures will end up with horizontal lines on them due to the 960 pixel e-ink paper screen width.
(I should have set up the pictures display vertically, then this change might not have been needed, being 540 pixels wide.)

*/

#include <M5EPD.h>
#include <memory>
#include <stdexcept>
#include <SD.h>

#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <JPEGDEC.h>

#define SLEEP_HOURS 1
bool dither = true;

JPEGDEC jpeg;

M5EPD_Canvas canvas(&M5.EPD);

const char *DATA_FILE = "/data.txt";
uint32_t lastCount;
uint8_t ditherSpace[15360];

//###############
// Callbacks for the jpeg decoding.

File myfile;

void * myOpen(const char *filename, int32_t *size) {
  myfile = SD.open(filename);
  *size = myfile.size();
  return &myfile;
}
void myClose(void *handle) {
  if (myfile) myfile.close();
}
int32_t myRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!myfile) return 0;
  return myfile.read(buffer, length);
}
int32_t mySeek(JPEGFILE *handle, int32_t position) {
  if (!myfile) return 0;
  return myfile.seek(position);
}

//###############

bool has_suffix(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool is_jpg(const std::string &filename) {
  return (has_suffix(filename, ".jpg") || has_suffix(filename, ".jpeg"));
}

bool is_valid_image(const std::string &filename) {
  return is_jpg(filename);
}

std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

void storeCountSD(uint32_t c) {
  auto f = SD.open(DATA_FILE, "wb");
  uint8_t buf[4];
  buf[0] = c;
  buf[1] = c >> 8;
  buf[2] = c >> 16;
  buf[3] = c >> 24;
  auto bytes = f.write(&buf[0], 4);
  f.close();
}

uint32_t getCountSD() {
  uint32_t val;
  if (SD.exists(DATA_FILE)) {
    auto f = SD.open(DATA_FILE, "rb");
    val = f.read();
    f.close();
  }
  else {
    val = 0;
  }
  return val;
}

void drawTempHumidityBattery() {
  char batteryBuffer[20];
  uint32_t vol = M5.getBatteryVoltage();
  if (vol < 3300) {
    vol = 3300;
  }
  else if (vol > 4350) {
    vol = 4350;
  }
  float battery = (float)(vol - 3300) / (float)(4350 - 3300);
  if (battery <= 0.01) {
    battery = 0.01;
  }
  if (battery > 1) {
    battery = 1;
  }
  sprintf(batteryBuffer, "%d%%", (int)(battery * 100));
  char statusBuffer[256] = "CHARGING";
  M5.SHT30.UpdateData();
  float tem = M5.SHT30.GetTemperature();
  float hum = M5.SHT30.GetRelHumidity();
  sprintf(statusBuffer, "%2.2fC | %0.2f%% | %s", tem, hum, batteryBuffer);
  canvas.drawRightString(statusBuffer, 960, 0, 1);
}

int JPEGDraw(JPEGDRAW *pDraw){
  uint16_t x = pDraw->x;
  uint16_t y = pDraw->y;
  uint16_t w = pDraw->iWidth;
  uint16_t h = pDraw->iHeight;
  uint16_t pixelPtr = 0;
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i += 4) {
      uint16_t col = pDraw->pPixels[ pixelPtr++ ];
      uint8_t col1 = 0xf - ((col >> 4) & 0xf);
      uint8_t col2 = 0xf - (col & 0xf);
      uint8_t col3 = 0xf - ((col >> 12) & 0xf);
      uint8_t col4 = 0xf - ((col >> 8) & 0xf);
      canvas.drawPixel(x + i, y + j, col1);
      canvas.drawPixel(x + i + 1, y + j, col2);
      canvas.drawPixel(x + i + 2, y + j, col3);
      canvas.drawPixel(x + i + 3, y + j, col4);
    } // for i
  } // for j
  return 1;
}

void load_image() {
  File root = SD.open("/");
  if (!root) {
    Serial.printf("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.printf("Not a directory");
    return;
  }
  String lastFile;
  lastCount = getCountSD();
  uint32_t currentCount = 0;
  while (true) {
    File file = root.openNextFile();
    if (!file) {
      Serial.printf("Reached end of directory, restart!\n");
      lastCount = 0;
      currentCount = 0;
      root.rewindDirectory();
      continue;
    }
    if (is_valid_image(file.name())) {
      currentCount++;
    }
    else {
      continue;
    }
    if (currentCount > lastCount) {
      if (is_jpg(file.name())) {
        Serial.printf("Filename %s\n", file.name());
        //canvas.drawJpgFile(SD, file.name(), 0, 0, 960, 540, 0, 0, JPEG_DIV_NONE);
        myfile = file;
        jpeg.open((const char *)file.name(), myOpen, myClose, myRead, mySeek, JPEGDraw);
        jpeg.setPixelType(FOUR_BIT_DITHERED);
        /*Decoder options
          JPEG_AUTO_ROTATE 1
          JPEG_SCALE_HALF 2
          JPEG_SCALE_QUARTER 4
          JPEG_SCALE_EIGHTH 8
          JPEG_LE_PIXELS 16
          JPEG_EXIF_THUMBNAIL 32
          JPEG_LUMA_ONLY 64
         */
        jpeg.decodeDither(ditherSpace, 0);
        jpeg.close();
      }
      else {
        Serial.printf("Something went wrong!\n");
        break;
      }
      lastCount = currentCount;
      canvas.drawRightString(file.name(), 960, 540, 1);
      break;
    }
  }
  storeCountSD(lastCount);
  drawTempHumidityBattery();
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}

void setup() {
  M5.begin();
  M5.EPD.SetRotation(0);
  M5.EPD.Clear(1);
  M5.RTC.begin();
  canvas.createCanvas(960, 540);
  canvas.setTextSize(2);
  load_image();
}

void loop() {
  int tick = 1000;
  while ((--tick) > 0) {
    delay(100);
    M5.update();
    if (M5.BtnL.wasPressed()) {
      Serial.println("Previous image.");
      tick = 0;
      uint32_t lastCount = getCountSD();
      lastCount -= 2;
      if ((lastCount) < 0) lastCount = 0;
      storeCountSD(lastCount);
    }
    if (M5.BtnP.wasPressed()) {
      Serial.println("Turning off.");
      canvas.drawRightString("Off", 960, 540, 1);
      canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
      delay(500);
      M5.shutdown();
    }
    if (M5.BtnR.wasPressed()) {
      Serial.println("Next image.");
      tick = 0;
    }
  }
  load_image();
}
