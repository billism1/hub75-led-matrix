#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

////////////////////////////////
// Adafruit_LIS3DH setup
////////////////////////////////

// // Used for software SPI
// #define LIS3DH_CLK 13   // SCL
// #define LIS3DH_MISO 12  // SD0
// #define LIS3DH_MOSI 11  // SDA
// // Used for hardware & software SPI
// #define LIS3DH_CS 10    // CS

// // Used for software SPI
// #define LIS3DH_CLK 32  // SCL
// #define LIS3DH_MISO 33 // SD0
// #define LIS3DH_MOSI 25 // SDA
// // Used for hardware & software SPI
// #define LIS3DH_CS 26 // CS

// software SPI
// Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS, LIS3DH_MOSI, LIS3DH_MISO, LIS3DH_CLK);
// hardware SPI
// Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS);
// I2C
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
static bool accelerometerFound = false;

enum Orientation
{
  UPRIGHT = 0,
  UPSIDE_DOWN = 1
};

Orientation orientation = Orientation::UPRIGHT;

////////////////////////////////
// End Adafruit_LIS3DH setup
////////////////////////////////

/*--------------------- MATRIX GPIO CONFIG  -------------------------*/
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN 18 // Needed fo 64x64. This is the only change from the default pins of the library
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16

/*--------------------- MATRIX PANEL CONFIG -------------------------*/
const int panelResX = MATRIX_PANEL_WIDTH;   // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = MATRIX_PANEL_HEIGHT;  // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = MATRIX_PANEL_CHAIN; // Total number of panels chained one to another

// Note about chaining panels:
// By default all matrix libraries treat the panels as been connected horizontally
// (one long display). The I2S Matrix library supports different display configurations
// Details here:

// See the setup method for more display config options
//------------------------------------------------------------------------------------------------------------------

MatrixPanel_I2S_DMA *dma_display = nullptr;

// Some standard colors
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myYELLOW = dma_display->color565(255, 255, 0);
uint16_t myCYAN = dma_display->color565(0, 255, 255);
uint16_t myMAGENTA = dma_display->color565(255, 0, 255);
uint16_t myBLACK = dma_display->color565(0, 0, 0);

//////////////////////
// Sand setup
//////////////////////

static const uint16_t ROWS = MATRIX_HEIGHT;
static const uint16_t COLS = MATRIX_WIDTH;

//////////////////////////////////////////
// Parameters you can play with:

int16_t millisToChangeColor = 250;
int16_t millisToChangeAllColors = 75;
int16_t millisToChangeInputX = 4000;

int16_t inputWidth = 3;
int16_t inputX = 4;
int16_t inputY = 0;
int16_t percentInputFill = 85;

// Maximum frames per second.
// The high the value, the faster the pixels fall.
unsigned long maxFps = 30;

int16_t maxVelocity = 7;
int16_t gravity = 1;
int16_t adjacentVelocityResetValue = 3;

// End parameters you can play with
//////////////////////////////////////////

struct GridState
{
  uint16_t state;
  uint16_t kValue;
  int16_t velocity;
};

/// Representation of an RGB pixel (Red, Green, Blue)
struct CRGB
{
  union
  {
    struct
    {
      union
      {
        uint8_t r;   ///< Red channel value
        uint8_t red; ///< @copydoc r
      };
      union
      {
        uint8_t g;     ///< Green channel value
        uint8_t green; ///< @copydoc g
      };
      union
      {
        uint8_t b;    ///< Blue channel value
        uint8_t blue; ///< @copydoc b
      };
    };
    /// Access the red, green, and blue data as an array.
    /// Where:
    /// * `raw[0]` is the red value
    /// * `raw[1]` is the green value
    /// * `raw[2]` is the blue value
    uint8_t raw[3];
  };
};

int16_t dropCount = 0;

unsigned long lastMillis;
unsigned long colorChangeTime = 0;
unsigned long allColorChangeTime = 0;

unsigned long inputXChangeTime = 0;

#define COLOR_BLACK 0
#define COLOR_MAGENTA 0xFF00FF

int16_t BACKGROUND_COLOR = COLOR_BLACK;

byte rgbValues[] = {0x31, 0x00, 0x00}; // red, green, blue
#define COLORS_ARRAY_RED rgbValues[0]
#define COLORS_ARRAY_GREEN rgbValues[1]
#define COLORS_ARRAY_BLUE rgbValues[2]

uint16_t newKValue = 0;

GridState **stateGrid;
GridState **nextStateGrid;
GridState **lastStateGrid;

static const uint16_t GRID_STATE_NONE = 0;
static const uint16_t GRID_STATE_NEW = 1;
static const uint16_t GRID_STATE_FALLING = 2;
static const uint16_t GRID_STATE_COMPLETE = 3;

static CRGB **pixels;

//////////////////////
// End Sand setup
//////////////////////

//////////////////////
// Sand functions.
//////////////////////

// Color changing state machine
void setNextColor(byte *rgbValues, uint16_t &kValue)
{
  switch (kValue)
  {
  case 0:
    rgbValues[1] += 2;
    if (rgbValues[1] == 64)
    {
      rgbValues[1] = 63;
      kValue = (uint16_t)1;
    }
    break;
  case 1:
    rgbValues[0]--;
    if (rgbValues[0] == 255)
    {
      rgbValues[0] = 0;
      kValue = (uint16_t)2;
    }
    break;
  case 2:
    rgbValues[2]++;
    if (rgbValues[2] == 32)
    {
      rgbValues[2] = 31;
      kValue = (uint16_t)3;
    }
    break;
  case 3:
    rgbValues[1] -= 2;
    if (rgbValues[1] == 255)
    {
      rgbValues[1] = 0;
      kValue = (uint16_t)4;
    }
    break;
  case 4:
    rgbValues[0]++;
    if (rgbValues[0] == 32)
    {
      rgbValues[0] = 31;
      kValue = (uint16_t)5;
    }
    break;
  case 5:
    rgbValues[2]--;
    if (rgbValues[2] == 255)
    {
      rgbValues[2] = 0;
      kValue = (uint16_t)0;
    }
    break;
  }
}

void setNextColor(uint16_t xCol, uint16_t yRow, uint16_t &kValue)
{
  CRGB *pixel = &(pixels[yRow][xCol]);
  setNextColor(pixel->raw, kValue);

  dma_display->drawPixelRGB888(xCol, yRow, pixel->red, pixel->green, pixel->blue);
}

bool withinCols(int16_t value)
{
  return value >= 0 && value <= COLS - 1;
}

bool withinRows(int16_t value)
{
  return value >= 0 && value <= ROWS - 1;
}

void setNextColorAll()
{
  for (int16_t i = 0; i < ROWS; ++i)
  {
    for (int16_t j = 0; j < COLS; ++j)
    {
      if (stateGrid[i][j].state != GRID_STATE_NONE)
      {
        // setNextColor(leds[getPanelXYOffset(j, i)].raw, stateGrid[i][j].kValue);
        setNextColor(j, i, stateGrid[i][j].kValue);
      }
    }
  }
}

// void resetAllPixels()
// {
//   Serial.println("Resetting all pixels.");

//   for (uint16_t i = 0; i < ROWS; ++i)
//   {
//     for (uint16_t j = 0; j < COLS; ++j)
//     {
//       if (stateGrid[i][j].state != GRID_STATE_NONE)
//       {
//         stateGrid[i][j].state = GRID_STATE_FALLING;
//         stateGrid[i][j].velocity = 1;
//       }
//     }
//   }

//   Serial.println("Done resetting all pixels.");
// }

bool isUpright()
{
  return orientation == Orientation::UPRIGHT;
}

int16_t axisAdd(int16_t a, int16_t b)
{
  return isUpright() ? (a + b) : (a - b);
}

int16_t axisSubtract(int16_t a, int16_t b)
{
  return isUpright() ? (a - b) : (a + b);
}

bool axisGreaterThan(int16_t a, int16_t b)
{
  return isUpright() ? (a > b) : (a < b);
}

void resetPixel(GridState **grid, uint16_t iRow, uint16_t jCol)
{
  grid[iRow][jCol].state = GRID_STATE_FALLING;
  grid[iRow][jCol].velocity = adjacentVelocityResetValue;
}

// Reset the pixels that "touch" given pixel. Up, down, left, right, and diagonal.
void resetAdjacentPixels(GridState **grid, int16_t x, int16_t y)
{
  for (auto yRow = y - 1; yRow <= y + 1; yRow++)
  {
    for (auto xCol = x - 1; xCol <= x + 1; xCol++)
    {
      if (yRow == y && xCol == x)
        continue;

      if (!withinRows(yRow) || !withinCols(xCol))
        continue;

      if (grid[yRow][xCol].state == GRID_STATE_COMPLETE)
        resetPixel(grid, yRow, xCol);
    }
  }
}

void resetColumnEdges(uint16_t jCol)
{
  for (uint16_t iRow = 0; iRow < ROWS; iRow++)
  {
    if (stateGrid[iRow][jCol].state == GRID_STATE_NONE)
      continue;

    if (!withinRows(iRow - 1) || !withinRows(iRow + 1))
    {
      resetPixel(stateGrid, iRow, jCol);
      //resetAdjacentPixels(stateGrid, iRow, jCol);
    }
    else if (withinRows(iRow - 1) && stateGrid[iRow - 1][jCol].state != GRID_STATE_COMPLETE)
    {
      resetPixel(stateGrid, iRow, jCol);
      //resetAdjacentPixels(stateGrid, iRow, jCol);
    }
    else if (withinRows(iRow + 1) && stateGrid[iRow + 1][jCol].state != GRID_STATE_COMPLETE)
    {
      resetPixel(stateGrid, iRow, jCol);
      //resetAdjacentPixels(stateGrid, iRow, jCol);
    }
  }
}

void resetEdgePixels()
{
  Serial.println("Resetting top pixels.");

  for (uint16_t jCol = 0; jCol < COLS; jCol++)
  {
    resetColumnEdges(jCol);
  }

  Serial.println("Done resetting top pixels.");
}

void setColor(uint16_t xCol, uint16_t yRow, int8_t _red, int8_t _green, int8_t _blue)
{
  CRGB *crgb = &(pixels[yRow][xCol]);

  crgb->raw[0] = _red;   /// * `raw[0]` is the red value
  crgb->raw[1] = _green; /// * `raw[1]` is the green value
  crgb->raw[2] = _blue;  /// * `raw[2]` is the blue value

  dma_display->drawPixelRGB888(xCol, yRow, _red, _green, _blue);
}

void resetGrid()
{
  // Serial.println("Initial values....");
  for (uint16_t i = 0; i < ROWS; ++i)
  {
    for (uint16_t j = 0; j < COLS; ++j)
    {
      // setColor(&leds[getPanelXYOffset(j, i)], 0, 0, 0);
      setColor(j, i, 0, 0, 0);

      stateGrid[i][j].state = GRID_STATE_NONE;
      stateGrid[i][j].velocity = 0;
      stateGrid[i][j].kValue = 0;
      nextStateGrid[i][j].state = GRID_STATE_NONE;
      nextStateGrid[i][j].velocity = 0;
      nextStateGrid[i][j].kValue = 0;
      lastStateGrid[i][j].state = GRID_STATE_NONE;
      lastStateGrid[i][j].velocity = 0;
      lastStateGrid[i][j].kValue = 0;
    }
  }

  dropCount = 0;
}

//////////////////////
// End sand funcitons.
//////////////////////

void displaySetup()
{
  HUB75_I2S_CFG mxconfig(
      panelResX,  // module width
      panelResY,  // module height
      panel_chain // Chain length
  );

  // This is how you enable the double buffer.
  // Double buffer can help with animation heavy projects
  // It's not needed for something simple like this, but some
  // of the other examples make use of it.

  // mxconfig.double_buff = true;

  // If you are using a 64x64 matrix you need to pass a value for the E pin
  // The trinity connects GPIO 18 to E.
  // This can be commented out for any smaller displays (but should work fine with it)
  mxconfig.gpio.e = 18;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  mxconfig.clkphase = false;

  // Some matrix panels use different ICs for driving them and some of them have strange quirks.
  // If the display is not working right, try this.
  // mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  // dma_display->setBrightness(255);
}

void accelerometerSetup()
{
  if (!lis.begin(0x18)) // change this to 0x19 for alternative i2c address
  {
    Serial.println("Couldn't start LIS3DH accelerometer.");
    return;
  }

  accelerometerFound = true;
  Serial.println("LIS3DH accelerometer found!");
  // lis.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!
  Serial.print("LIS3DH accelerometer range = ");
  Serial.print(2 << lis.getRange());
  Serial.println("G");

  // lis.setDataRate(LIS3DH_DATARATE_50_HZ);
  Serial.print("LIS3DH accelerometer data rate set to: ");
  switch (lis.getDataRate())
  {
  case LIS3DH_DATARATE_1_HZ:
    Serial.println("1 Hz");
    break;
  case LIS3DH_DATARATE_10_HZ:
    Serial.println("10 Hz");
    break;
  case LIS3DH_DATARATE_25_HZ:
    Serial.println("25 Hz");
    break;
  case LIS3DH_DATARATE_50_HZ:
    Serial.println("50 Hz");
    break;
  case LIS3DH_DATARATE_100_HZ:
    Serial.println("100 Hz");
    break;
  case LIS3DH_DATARATE_200_HZ:
    Serial.println("200 Hz");
    break;
  case LIS3DH_DATARATE_400_HZ:
    Serial.println("400 Hz");
    break;

  case LIS3DH_DATARATE_POWERDOWN:
    Serial.println("Powered Down");
    break;
  case LIS3DH_DATARATE_LOWPOWER_5KHZ:
    Serial.println("5 Khz Low Power");
    break;
  case LIS3DH_DATARATE_LOWPOWER_1K6HZ:
    Serial.println("16 Khz Low Power");
    break;
  }
}

void updateAccelerometerData()
{
  if (!accelerometerFound)
    return;

  // Accelerometer
  // lis.read(); // get X Y and Z data at once
  // // Then print out the raw data
  // Serial.println("LIS3DH accelerometer data:");
  // Serial.print("X:  ");
  // Serial.print(lis.x);
  // Serial.print("  \tY:  ");
  // Serial.print(lis.y);
  // Serial.print("  \tZ:  ");
  // Serial.print(lis.z);
  // Serial.println();
  // Serial.println();

  /* Or....get a new sensor event, normalized */
  sensors_event_t event;
  lis.getEvent(&event);

  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("\t\tX: ");
  Serial.print(event.acceleration.x);
  Serial.print(" \tY: ");
  Serial.print(event.acceleration.y);
  Serial.print(" \tZ: ");
  Serial.print(event.acceleration.z);
  Serial.println(" m/s^2 ");
  Serial.println();
  Serial.println();

  Orientation previousOrientation = orientation;

  // Ranges are 10 to -10 for each axis.
  // 0 Z        = tipped at 90 degrees (non-zero positive number means it is upright, but tipped)
  // negative X = tipped to the right
  // positive X = tipped to the left

  if (event.acceleration.z >= 0)
  { // 0 to 180 degrees, (Upside-up)
    orientation = Orientation::UPRIGHT;

    if (event.acceleration.x > 0)
    {
      // Tipping left
    }
    else
    {
      // Tipping right
      if (event.acceleration.x < 0 && event.acceleration.x > -5.5)
      {
        // Tilting right
      }
      else if (event.acceleration.x <= -5.5 && event.acceleration.x <= -10)
      {
        // Tilting right
      }
    }

    // if (event.acceleration.y > 0)
    // {
    //   // Tipping back
    // }
    // else
    // {
    //   // Tipping forward
    // }
  }
  else
  { // 181 to 360 degrees (Upside-down)
    orientation = Orientation::UPSIDE_DOWN;

    if (event.acceleration.x > 0)
    {
      // ?
    }
    else
    {
      // ?
    }

    // if (event.acceleration.y > 0)
    // {
    //   // ?
    // }
    // else
    // {
    //   // ?
    // }
  }

  if (orientation != previousOrientation)
  {
    resetEdgePixels();
  }
}

void setup()
{
  Serial.begin(115200);

  accelerometerSetup();

  displaySetup();

  // delay(3000);

  // Init 2-d arrays, holding pixel state
  // Serial.println("Init other 2-d arrays, holding pixel state....");
  stateGrid = new GridState *[ROWS];
  nextStateGrid = new GridState *[ROWS];
  lastStateGrid = new GridState *[ROWS];
  pixels = new CRGB *[ROWS];

  for (int16_t i = 0; i < ROWS; ++i)
  {
    stateGrid[i] = new GridState[COLS];
    nextStateGrid[i] = new GridState[COLS];
    lastStateGrid[i] = new GridState[COLS];
    pixels[i] = new CRGB[COLS];
  }

  // Initial values
  resetGrid();

  colorChangeTime = millis() + 1000;

  lastMillis = millis();
}

void loop()
{
  // Serial.println("Looping...");

  // Change the color of the new pixels over time
  if (colorChangeTime < millis())
  {
    colorChangeTime = millis() + millisToChangeColor;
    setNextColor(rgbValues, newKValue);
  }

  // Change the color of the fallen pixels over time
  if (allColorChangeTime < millis())
  {
    allColorChangeTime = millis() + millisToChangeAllColors;
    setNextColorAll();
  }

  unsigned long currentMillis = millis();
  unsigned long diffMillis = currentMillis - lastMillis;

  if ((1000 / maxFps) > diffMillis)
  {
    return;
  }

  lastMillis = currentMillis;
  // Serial.println("Looping within FPS limit...");

  updateAccelerometerData();

  inputY = isUpright() ? 0 : ROWS - 1;

  // Change the inputX of the pixels over time or if the current input is already filled.
  if (inputXChangeTime < millis())
  {
    inputXChangeTime = millis() + millisToChangeInputX;
    inputX = random(0, COLS);
  }

  // Randomly add an area of pixels
  int16_t halfInputWidth = inputWidth / 2;
  for (int16_t i = -halfInputWidth; i <= halfInputWidth; ++i)
  {
    for (int16_t j = -halfInputWidth; j <= halfInputWidth; ++j)
    {
      if (random(100) < percentInputFill)
      {
        dropCount++;
        if (dropCount > (inputWidth * ROWS * COLS))
        {
          dropCount = 0;
          resetGrid();
        }

        int16_t col = inputX + i;
        int16_t row = inputY + j;

        if (withinCols(col) && withinRows(row) &&
            stateGrid[row][col].state == GRID_STATE_NONE)
        //(stateGrid[row][col].state == GRID_STATE_NONE || stateGrid[row][col].state == GRID_STATE_COMPLETE))
        {
          setColor(col, row, COLORS_ARRAY_RED, COLORS_ARRAY_GREEN, COLORS_ARRAY_BLUE);
          stateGrid[row][col].state = GRID_STATE_NEW;
          stateGrid[row][col].velocity = 1;
          stateGrid[row][col].kValue = newKValue;
        }
      }
    }
  }

  // Clear the next state frame of animation
  for (uint16_t i = 0; i < ROWS; ++i)
  {
    for (uint16_t j = 0; j < COLS; ++j)
    {
      nextStateGrid[i][j].state = GRID_STATE_NONE;
      nextStateGrid[i][j].velocity = 0;
      nextStateGrid[i][j].kValue = 0;
    }
  }

  // Serial.println("Checking every cell...");
  // unsigned long beforeCheckEveryCell = millis();
  // unsigned long pixelsChecked = 0;
  // Check every pixel to see which need moving, and move them.
  for (int16_t i = 0; i < ROWS; ++i)
  {
    int16_t j = isUpright() ? 0 : COLS - 1;
    auto compareJ = [&j]()
    {
      return isUpright() ? (j < COLS) : (j >= 0);
    };

    // If iteration starting column is not altered on Z axis change, pixels tend to pile towards one side on reperated Z axis changes.
    for (; compareJ(); j = axisAdd(j, 1))
    {
      // This nexted loop is where the bulk of the computations occur.
      // Tread lightly in here, and check as few pixels as needed.

      // Get the state of the current pixel.
      CRGB pixelColor = pixels[i][j];
      uint16_t pixelState = stateGrid[i][j].state;
      int16_t pixelVelocity = stateGrid[i][j].velocity;
      uint16_t pixelKValue = stateGrid[i][j].kValue;

      bool moved = false;

      // If the current pixel has landed, no need to keep checking for its next move.
      if (pixelState != GRID_STATE_NONE && pixelState != GRID_STATE_COMPLETE)
      {
        // pixelsChecked++;

        int16_t newPos = int16_t(axisAdd(i, min(maxVelocity, pixelVelocity)));
        for (int16_t y = newPos; axisGreaterThan(y, i); y = axisSubtract(y, 1))
        {
          if (!withinRows(y))
          {
            continue;
          }

          GridState belowState = stateGrid[y][j];
          GridState belowNextState = nextStateGrid[y][j];

          int16_t direction = 1;
          if (random(100) < 50)
          {
            direction *= -1;
          }

          GridState *belowStateA = NULL;
          GridState *belowNextStateA = NULL;
          GridState *belowStateB = NULL;
          GridState *belowNextStateB = NULL;

          if (withinCols(j + direction))
          {
            belowStateA = &stateGrid[y][j + direction];
            belowNextStateA = &nextStateGrid[y][j + direction];
          }
          if (withinCols(j - direction))
          {
            belowStateB = &stateGrid[y][j - direction];
            belowNextStateB = &nextStateGrid[y][j - direction];
          }

          auto randomishVelocityGain = random(gravity, gravity + 2);

          if (belowState.state == GRID_STATE_NONE && belowNextState.state == GRID_STATE_NONE)
          {
            // This pixel will go straight down.
            setColor(j, y, pixelColor.red, pixelColor.green, pixelColor.blue);

            nextStateGrid[y][j].state = GRID_STATE_FALLING;
            nextStateGrid[y][j].velocity = pixelVelocity + randomishVelocityGain;
            nextStateGrid[y][j].kValue = pixelKValue;
            moved = true;
            break;
          }
          else if ((belowStateA != NULL && belowStateA->state == GRID_STATE_NONE) && (belowNextStateA != NULL && belowNextStateA->state == GRID_STATE_NONE))
          {
            // This pixel will fall to side A (right)
            setColor(j + direction, y, pixelColor.red, pixelColor.green, pixelColor.blue);

            nextStateGrid[y][j + direction].state = GRID_STATE_FALLING;
            nextStateGrid[y][j + direction].velocity = pixelVelocity + randomishVelocityGain;
            nextStateGrid[y][j + direction].kValue = pixelKValue;
            moved = true;
            break;
          }
          else if ((belowStateB != NULL && belowStateB->state == GRID_STATE_NONE) && (belowNextStateB != NULL && belowNextStateB->state == GRID_STATE_NONE))
          {
            // This pixel will fall to side B (left)
            setColor(j - direction, y, pixelColor.red, pixelColor.green, pixelColor.blue);

            nextStateGrid[y][j - direction].state = GRID_STATE_FALLING;
            nextStateGrid[y][j - direction].velocity = pixelVelocity + randomishVelocityGain;
            nextStateGrid[y][j - direction].kValue = pixelKValue;
            moved = true;
            break;
          }
        }
      }

      if (moved)
      {
        // Reset color where this pixel was.
        // setColor(&leds[getPanelXYOffset(j, i)], 0, 0, 0);
        setColor(j, i, 0, 0, 0);

        resetAdjacentPixels(nextStateGrid, j, i);
      }

      if (pixelState != GRID_STATE_NONE && !moved)
      {
        nextStateGrid[i][j].velocity = pixelVelocity + gravity;
        nextStateGrid[i][j].kValue = pixelKValue;

        if (pixelState == GRID_STATE_NEW)
          nextStateGrid[i][j].state = GRID_STATE_FALLING;
        // TODO: Better way (than increasing velocity to 20 while the pixel is stopped) to allow pixels to be "reset" on orientation change.
        else if (pixelState == GRID_STATE_FALLING && pixelVelocity > 20)
          nextStateGrid[i][j].state = GRID_STATE_COMPLETE;
        else
          nextStateGrid[i][j].state = pixelState; // should be GRID_STATE_COMPLETE
      }
    }
  }
  // unsigned long afterCheckEveryCell = millis();
  // unsigned long checkCellTime = afterCheckEveryCell - beforeCheckEveryCell;
  // Serial.printf("%lu pixels checked in %lu ms.\r\n", pixelsChecked, checkCellTime);

  // Swap the state pointers.

  lastStateGrid = stateGrid;
  stateGrid = nextStateGrid;
  nextStateGrid = lastStateGrid;
}
