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
#define _WaitingTimeStart 1000

// _BackgroundBrightness fills the border on small images: 0: White... 15: black
#define _BackgroundBrightness 0

JPEGDEC jpeg;

M5EPD_Canvas canvas(&M5.EPD);

bool dither = true;
const char *DATA_FILE = "/data.txt";
uint32_t lastCount;
uint8_t ditherSpace[15360];
//uint16_t _WaitingTimeStart = 1000;
uint16_t waitingTime = 1000;
int offsetX = 0; // Screen space centering.
int offsetY = 0; // Screen space centering.


//###############
// Callbacks for the jpeg decoding.

File globalFileHandle;

void * myOpen(const char *filename, int32_t *size) {
  globalFileHandle = SD.open(filename);
  *size = globalFileHandle.size();
  return &globalFileHandle;
}
void myClose(void *handle) {
  if (globalFileHandle) globalFileHandle.close();
}
int32_t myRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!globalFileHandle) return 0;
  return globalFileHandle.read(buffer, length);
}
int32_t mySeek(JPEGFILE *handle, int32_t position) {
  if (!globalFileHandle) return 0;
  return globalFileHandle.seek(position);
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
  // Get
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

int JPEGDraw(JPEGDRAW *pDraw) {
  // offsetX and Y are globaly defined values just before drawing,
  // for centering in this callback function..
  uint16_t x = pDraw->x + offsetX;
  uint16_t y = pDraw->y + offsetY;
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

void loadNextImage() {
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
      continue; // Back to top of while(true);
    }
    if (is_jpg(file.name())) {
      currentCount++;
    }
    else {
      continue; // Back to top of while(true);
    }
    if (currentCount > lastCount) {
      Serial.printf("Filename %s\n", file.name());
      globalFileHandle = file; // Set the global variable for the JPG decode callback.
      lastCount = currentCount;
      storeCountSD(lastCount);
      drawImage((char *) file.name());
      break; // End the while(true) loop.
    } // if (currentCount > lastCount)
  } // while (true)
}

void drawImage(char *fileName) {

  uint8_t result = jpeg.open(fileName, myOpen, myClose, myRead, mySeek, JPEGDraw);
  if (result != 1) {
    uint8_t err = jpeg.getLastError();
    switch (err) {
      case 1: Serial.println("Error: JPEG_INVALID_PARAMETER"); break; //progressive JPG not supported"); break;
      case 2: Serial.println("Error: JPEG_DECODE_ERROR"); break;
      case 3: Serial.println("Error: JPEG_UNSUPPORTED_FEATURE (progressive JPG's not supported)"); break;
      case 4: Serial.println("Error: JPEG_INVALID_FILE"); break;
    }
    waitingTime = 1;
    return;
  }

  int width = jpeg.getWidth();
  int height = jpeg.getHeight();

  Serial.print("Image size: ");
  Serial.print(width);
  Serial.print(" x ");
  Serial.println(height);

  // Too big? Then scale.
  uint8_t scaling = 0; // Get's or'd with options for the following:
  uint8_t options = 0;
  /*Decoder options
    JPEG_AUTO_ROTATE 1
    JPEG_SCALE_HALF 2
    JPEG_SCALE_QUARTER 4
    JPEG_SCALE_EIGHTH 8
    JPEG_LE_PIXELS 16
    JPEG_EXIF_THUMBNAIL 32
    JPEG_LUMA_ONLY 64
  */
  if (width > 960 || height > 540) {
    // Try half size.
    width >>= 1;
    height >>= 1;
    Serial.print("Trying half size: ");
    Serial.print(width);
    Serial.print(" x ");
    Serial.println(height);
    scaling = JPEG_SCALE_HALF;
    if (width > 960 || height > 540) {
      // Try quarter size.
      width >>= 1;
      height >>= 1;
      scaling = JPEG_SCALE_QUARTER;
      Serial.print("Trying quarter size: ");
      Serial.print(width);
      Serial.print(" x ");
      Serial.println(height);
      if (width > 960 || height > 540) {
        // Try eigth size.
        width >>= 1;
        height >>= 1;
        scaling = JPEG_SCALE_EIGHTH;
        Serial.println("Trying eighth size: ");
        Serial.print(width);
        Serial.print(" x ");
        Serial.println(height);
        if (width > 960 || height > 540) {
          Serial.println("Still too big after scaling attempt, skipping.");
          // Still too big, cancel operation, and don't pause for any length of time for next image.
          waitingTime = 1;
          return;
        } // Too big.
      } // Eighth size.
    } // Quarter size.
    Serial.println("Resized image now fits.");
  } // Half size.
  // For when this image is smaller than the last - make sure no old image can be seen on the boundaries.
  canvas.fillCanvas(_BackgroundBrightness);
  offsetX = (960 - width) >> 1;
  offsetY = (540 - height) >> 1;
  jpeg.setPixelType(FOUR_BIT_DITHERED);
  jpeg.decodeDither(ditherSpace, options | scaling);
  jpeg.close();
  canvas.drawRightString(fileName, 960, 540, 1);
  drawTempHumidityBattery();
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
  waitingTime = _WaitingTimeStart;
}

void setup() {
  M5.begin();
  M5.EPD.SetRotation(0);
  M5.EPD.Clear(1);
  M5.RTC.begin();
  canvas.createCanvas(960, 540);
  canvas.setTextSize(2);
  loadNextImage();
  waitingTime = _WaitingTimeStart;
}

void loop() {
  while ((--waitingTime) > 0) {
    delay(100);
    M5.update();
    if (M5.BtnL.wasPressed()) {
      Serial.println("Previous image.");
      waitingTime = 1;
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
      waitingTime = 1;
    }
  }
  loadNextImage();
}
