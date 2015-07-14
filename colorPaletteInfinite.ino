// This #include statement was automatically added by the Spark IDE.
#include "FastLED/FastLED.h"
FASTLED_USING_NAMESPACE;

/*

Color Palette Infinite!

This sketched will be used to send simple instructions
to Cores, Photons, and Electrons running complex animations.

The information about the animations is vastly simplified by the 
color palette object, which is used to create the structure of the animation
while animation functions are used to move and change that palette over time

This is a work in progress and makes no claim to be the simplest way to solve this problem,
but it will work for our application andcould be helpful to reference

Structure:
[HSVhsvApp]

HSV - Layout configurations in hue, saturation, and value
hsv - Baseline HSV color to "seed" the animation
A - Animation pattern to map the palette to the LEDs over time
pp - Optional parameters for the animate function

*/


// LED mappings
#define LED_PIN_A     4
#define LED_PIN_B     5
#define LED_PIN_C     6
#define LED_PIN_D     7
#define NUM_STRIPS   4
#define BRIGHTNESS   255
#define LED_TYPE     WS2812
#define COLOR_ORDER  GRB
const uint8_t kMatrixWidth  = 16;
const uint8_t kMatrixHeight = 16;
const bool    kMatrixSerpentineLayout = true;
const uint8_t ledsPerStrip  = kMatrixWidth * kMatrixHeight / NUM_STRIPS;
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)
CRGB leds[kMatrixWidth * kMatrixHeight];
CRGB prev_leds[kMatrixWidth * kMatrixHeight];
CRGB current_leds[kMatrixWidth * kMatrixHeight];
uint8_t ledBrightness[NUM_LEDS];

// Timing controls
const int     refreshRate = 120; // Hz                 // Refresh rate of the LEDs independent of counters
const long    refreshIncrement = 1000 / refreshRate;   // Refresh rate in milliseconds
unsigned long previousMillis = 0;                      // will store last time LEDs were updated
uint16_t      count16 = 0;                             // 16 bit counter to reference in animations
uint8_t       count8  = 0;                             // 8 bit counter to reference in animations
uint8_t       frameBlendCounter = 0;                   // 8 bit counter to reference in animations
long          countMinutes  = 0;                       // Out of 1440 minutes per day
#define MINUTES_SPEED       1                          // Number of counts before advancing 1 "minute"

// External animation trigger controls
#define FRAME_ADV_PIN       1
#define ANIMATION_ADV_PIN   2
const long animationDebounce = 2000;
const bool    isTriggered = false;
bool          isRandom = true;

// Array storing mode data for the animations
uint8_t currentMode[20];
uint8_t previousModes[10][20];
uint8_t mainColor[3];
uint8_t secondaryColor[3];
uint8_t animationParams[3];
uint8_t palettePatterns[3][16];
uint8_t paletteSpeed = 128;     // The speed that the palette is animated at
uint8_t paletteDensity = 1;     // Ratio of palette position to LEDs
uint8_t animationSpeed = 128;   // The speed that the animations advance
TBlendType    currentBlending = BLEND;
NSFastLED::CHSV baseColor1 = NSFastLED::CHSV( 192, 255, 255 );
NSFastLED::CHSV baseColor2 = NSFastLED::CHSV( 224, 230, 255 );

uint8_t       countMode  = 1;
bool          modeButtonState = false;
bool          restartState = 1;   // Initialize the app if restarted



CRGBPalette16 currentPalette( OceanColors_p );
CRGBPalette16 targetPalette( CRGB::Black );
uint8_t       countPalette = 0;   // Count which palette to animate from
uint8_t       countAnimation = 0;   // Count which position to animate from
uint8_t       colorLoop = 1;
uint8_t       cloudiness = 130;
uint8_t       maxChanges = 64; 

// 1 = 5 sec per palette
// 2 = 10 sec per palette
// etc
#define HOLD_PALETTES_X_TIMES_AS_LONG 4





// The 16 bit version of our coordinates
static uint16_t x;
static uint16_t y;
static uint16_t z;

// We're using the x/y dimensions to map to the x/y pixels on the matrix.  We'll
// use the z-axis for "time".  speed determines how fast time moves forward.  Try
// 1 for a very slow moving effect, or 60 for something that ends up looking like
// water.
uint16_t speed = 20; // speed is set dynamically once we've started up

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.
uint16_t scale = 30; // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];






void setup() {
    delay(3000);
    
    /*
    LEDS.addLeds<LED_TYPE,LED_PIN_A,COLOR_ORDER>(current_leds, 0, ledsPerStrip);
    LEDS.addLeds<LED_TYPE,LED_PIN_B,COLOR_ORDER>(current_leds, ledsPerStrip, ledsPerStrip);
    LEDS.addLeds<LED_TYPE,LED_PIN_C,COLOR_ORDER>(current_leds, 2 * ledsPerStrip, ledsPerStrip);
    LEDS.addLeds<LED_TYPE,LED_PIN_D,COLOR_ORDER>(current_leds, 3 * ledsPerStrip, ledsPerStrip);
    */
    
    LEDS.addLeds<LED_TYPE,LED_PIN_A,COLOR_ORDER>(current_leds, 0, 2 * ledsPerStrip);
    LEDS.addLeds<LED_TYPE,LED_PIN_B,COLOR_ORDER>(current_leds, 2 * ledsPerStrip, 2 * ledsPerStrip);
    LEDS.addLeds<LED_TYPE,LED_PIN_C,COLOR_ORDER>(current_leds, 0, 2 * ledsPerStrip);
    LEDS.addLeds<LED_TYPE,LED_PIN_D,COLOR_ORDER>(current_leds, 2 * ledsPerStrip, 2 * ledsPerStrip);
    
    Spark.subscribe("universe_mode_commands_1a", updateMode, MY_DEVICES);
    Spark.function("mode",setMode);
    
    LEDS.setBrightness(BRIGHTNESS);
    
    randomSeed(Time.second());
    
    // Initialize noising coordinates to some random values
    x = random16();
    y = random16();
    z = random16();
    
    
    pinMode(FRAME_ADV_PIN, INPUT_PULLDOWN);
    pinMode(ANIMATION_ADV_PIN, INPUT_PULLDOWN);
    
    bool setupFromString = false;
    if (setupFromString == true) {
        // Set the default values for the current mode
        setMode(String("9440041100111zzzzzz0"));
    } else {
        currentMode[0] = 13;     // Palette Control
        currentMode[1] = 0;      // Animation Control
        currentMode[2] = 1;    // Animation Speed
        currentMode[3] = 0;      // Animation Param1
        currentMode[4] = 0;      // Animation Param2
        currentMode[5] = 1;    // Palette Speed
        currentMode[6] = 1;      // Palette Density
        currentMode[7] = 1;      // Palette isBlended
        currentMode[8] = 0;      // Palette Param1
        currentMode[9] = 0;      // Palette Param2
        currentMode[10] = 1;     // Main Hue Animation
        currentMode[11] = 1;     // Main Sat Animation
        currentMode[12] = 1;     // Main Val Animation
        currentMode[13] = 192;   // Main Hue
        currentMode[14] = 220;   // Main Sat
        currentMode[15] = 255;   // Main Val
        currentMode[16] = 224;   // Secondary Hue
        currentMode[17] = 255;   // Secondary Sat
        currentMode[18] = 255;   // Secondary Val
        currentMode[19] = 0;     // Currently unused
    }
      
    if (isTriggered == true) {
        while (digitalRead(FRAME_ADV_PIN) < 1) {
          SetupRandomPalette();
          fillnoise8();
          mapNoiseToLEDsUsingPalette();
          LEDS.show();
        }
    }

}


void loop() {
  
  // Update the LEDs with the designated refresh rate
  unsigned long currentMillis = millis();
  if (isTriggered == true) {
    // Update when triggered and wait a bit before triggering again
    if(currentMillis - previousMillis > animationDebounce && digitalRead(FRAME_ADV_PIN) > 0) {
      updateAll(currentMillis);
    }
  } else {
    // Continuously update with the refresh rate
    if(currentMillis - previousMillis > refreshIncrement) {
      updateAll(currentMillis);
    }
  }
  // delay(10);
}

void updateAll(unsigned long currentMillis) {
    if (frameBlendCounter == 0) {
        // Save the previous configuration of the LEDs
        for (int i=0; i<NUM_LEDS; i++) {
            prev_leds[i] = leds[i];
        }
        
        updateCounters(currentMillis);
        
        if (count8 == 0  && isRandom==true) {
            randomMode();
            RandomizeSaturationAndValue();
        }
        
        updatePalettes();
        updateAnimations();
        
        // Initialize the targetPalette so it isn't left black
        if (restartState == 1) {
            restartState = 0;
            targetPalette = currentPalette;
        }
        
        
    } else {
        // Blend into the next LED animation
        for (int i=0; i<NUM_LEDS; i++) {
            current_leds[i] = blend(prev_leds[i], leds[i], frameBlendCounter);
        }
    }
    
    
    // Show the LEDs and update the animation counter
    LEDS.show();
    frameBlendCounter = (frameBlendCounter + 16) % 256;
}

void updateCounters(unsigned long currentMillis) {
    
    // First, update all of the Mode variables
    animationSpeed = currentMode[2];   // The speed that the palette is mapped to the LEDs
    
    paletteSpeed = currentMode[5];     // The speed that the palette is animated at
    paletteDensity = currentMode[6];   // The ratio of palette position to LED position
    if (currentMode[7] == 0) {currentBlending = NOBLEND;} else {currentBlending = BLEND;}
    
    
    for (int i=0; i<3; i++) {
        mainColor[i] = currentMode[i+13];
        secondaryColor[i] = currentMode[i+16];
        animationParams[i] = currentMode[i+10];
    }
    baseColor1 = NSFastLED::CHSV( mainColor[0],  mainColor[1], mainColor[2] );
    baseColor2 = NSFastLED::CHSV( secondaryColor[0],  secondaryColor[1], secondaryColor[2] );
	
    // Update the main 8-bit counnter
    count16 += 1;
    count8 = count16 % 256;
    
    // Advance the minutes of the day
    countMinutes = (countMinutes + MINUTES_SPEED) % 5760;
    
    // Update the refresh rate counter
    previousMillis = currentMillis;  
    
    //Advance the position of the palette over the LEDs if animation is called for
    if (animationParams[0] > 0) {
        countPalette = ((count16/256) * paletteSpeed) % 256;
    } else {
        countPalette = mainColor[0];
    }
    countAnimation = (count8 * animationSpeed);
}


void updatePalettes() {
    
    /*
    if (digitalRead(ANIMATION_ADV_PIN) == HIGH && modeButtonState == false) {
    countMode = (countMode + 1) % 12;
    modeButtonState = true;
    } else if (digitalRead(ANIMATION_ADV_PIN) == LOW) {
    modeButtonState = false;
    }
    */
    
    switch (currentMode[0]) {
        case 0:
            SetupSunrisePalette();
            break;
        case 1:
            SetupSunnyCloudsPalette();
            break;
        case 2:
            SetupSunsetPalette();  
            break;
        case 3:
            SetupMoonlightPalette();
            break;
        case 4:
            SetupAnalogousPalette();
            break;
        case 5:
            SetupComplementaryPalette();
            break;
        case 6:
            SetupSplitComplementaryPalette();
            break;
        case 7:
            SetupDoubleComplementaryPalette();
            break;
        case 8:
            SetupTriadicPalette();
            break;
        case 9:
            SetupRandomPalette();
            break;
        case 10:
            SetupCrystalPalette();
            break;
        case 11:
            SetupAlternatingPalette(mainColor[0], mainColor[1], secondaryColor[0], secondaryColor[1]); 
            break;
        case 12:
            SetupRangedPalette();
            break;
        case 13:
            RandomizeCustomPalette();
            break;
        default:
            SetupSunnyCloudsPalette();
    }
    nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);
    
}

void updateAnimations() {
    // convert the noise data to colors in the LED array
    // using the current palette
    
    switch (currentMode[1]) {
        case 0:
            FillLEDsFromPaletteColors( countAnimation );
            break;
        case 1:
            // generate noise data slower than the default allows
            fillnoise8();
            mapNoiseToLEDsUsingPalette();
            break;
        case 2:
            FillLEDsWithRandomBrightness( countAnimation );
            break;
        case 3:
            FillLEDsByBlinkingOn( countAnimation );
            break;
        case 4:
            FillLEDsByBlinkingAndFading( countAnimation );
            break;
        case 5:
            TwinkleFromPalette( countAnimation );
            break;
        default:
            FillLEDsFromPaletteColors( countAnimation );
        
    }
}

// Listen for events on the subscribed channel and update the mode values
void updateMode(const char *event, const char *data) {
    
    // data will be 9 ascii characters 
    // to be parsed into the 9 uint8_t values in currentMode
    
}

int setMode(String modeCommand) {
    if (modeCommand.length() == 20) {
        for (int i = 0; i < 9; i++) {
            previousModes[0][i] = currentMode[i];
            currentMode[i] = (modeCommand.charAt(i) - 48);  // Set ASCII 0 as the zero for these commands
        }
        count8 = 254;  // start blending in the new palette soon after the update
        return 1;
    } else {
        return -1;
    }
}

void randomMode() {
    if (animationParams[0] > 0) {
        countPalette = random8();
    } else {
        countPalette = mainColor[0];
    }
    
    currentMode[0] = random(4,12);
    currentMode[1] = random(0,5);
}


/*

================================
===   Animation Functions    ===
================================

*/



void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
  uint8_t brightness = 255;
  
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += paletteDensity;
  }
}

void FillLEDsWithRandomBrightness( uint8_t colorIndex)
{
  
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( currentPalette, random8(), random8(), currentBlending);
    colorIndex += paletteDensity;
  }
}

void FillLEDsByBlinkingOn( uint8_t colorIndex)
{
  uint8_t brightness = 255;
  
  for( int i = 0; i < NUM_LEDS; i++) {
    if (random8() > 230) {
        brightness = 255;
    } else {
        brightness = 0;
    }
    leds[i] = ColorFromPalette( currentPalette, random8(), brightness, currentBlending);
    colorIndex += paletteDensity;
  }
  blur1d(leds, NUM_LEDS, 64);
  //blur2d(leds, kMatrixWidth, kMatrixHeight, 64);
}

void FillLEDsByBlinkingAndFading( uint8_t colorIndex)
{
  for( int i = 0; i < NUM_LEDS; i++) {
    if (random8() > 250) {
        ledBrightness[i] = 255;
    } else {
        if (ledBrightness[i] >= 5) {ledBrightness[i] -= 5;}
    }
    leds[i] = ColorFromPalette( currentPalette, colorIndex, ledBrightness[i], currentBlending);
    colorIndex += paletteDensity;
  }
  blur1d(leds, NUM_LEDS, 64);
  //blur2d(leds, kMatrixWidth, kMatrixHeight, 64);
}

void TwinkleFromPalette( uint8_t colorIndex) 
{
  uint8_t brightness = 255;
  
  for( int i = 0; i < NUM_LEDS; i++) {
    if (random8() > 230) {
        leds[i] = baseColor1;
    } else {
        leds[i] = ColorFromPalette( currentPalette, random8(), brightness, currentBlending);
    }
    colorIndex += paletteDensity;
  }
  blur1d(leds, NUM_LEDS, 64);
  //blur2d(leds, kMatrixWidth, kMatrixHeight, 64);
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if( speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }
  
  for(int i = 0; i < MAX_DIMENSION; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < MAX_DIMENSION; j++) {
      int joffset = scale * j;
      
      uint8_t data = inoise8(x + ioffset,y + joffset,z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));

      if( dataSmoothing ) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }
      
      noise[i][j] = data;
    }
  }
  
  z += speed;
  
  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8;
  y -= speed / 16;
}

void mapNoiseToLEDsUsingPalette()
{
  static uint8_t ihue=0;
  
  for(int i = 0; i < kMatrixWidth; i++) {
    for(int j = 0; j < kMatrixHeight; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[j][i];
      uint8_t bri =   noise[i][j];

      // if this palette is a 'loop', add a slowly-changing base value
      if( colorLoop) { 
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the 
      // light/dark dynamic range desired
      if( bri > 127 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }

      CRGB color = ColorFromPalette( currentPalette, index, bri);
      leds[XY(i,j)] = color;
    }
  }
  
  ihue+=1;
}


//
// Mark's xy coordinate mapping code.  See the XYMatrix for more information on it.
//
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;
  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }
  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }
  return i;
}




/*

==============================
===   Palette Functions    ===
==============================





*/

void RandomizeCustomPalette() {
    if (count8<1) {
    // Build a custom animation in Hue, Saturation, and Value
    buildPaletteArray(random8(2,6), 0, random8());
    buildPaletteArray(random8(6), 1, random8());
    buildPaletteArray(random8(6), 2, random8(64,255));
    SetupCustomPalette();
    }
}

void RandomizeSaturationAndValue() {
    if (count8<1) {
    // Build a custom animation in Hue, Saturation, and Value
    for (int i=0; i<16; i++) {
        palettePatterns[0][i] = rgb2hsv_approximate( targetPalette[i] ).hue;
    }
    buildPaletteArray(random8(6), 1, random8());
    buildPaletteArray(random8(6), 2, random8(64,255));
    
    for (int i=0; i<16; i++) {
        targetPalette[i] = NSFastLED::CHSV( palettePatterns[0][i], palettePatterns[1][i], palettePatterns[2][i]);
    }
    }
}

void SetupCustomPalette() {
    for (int i=0; i<16; i++) {
        targetPalette[i] = NSFastLED::CHSV( palettePatterns[0][i], palettePatterns[1][i], palettePatterns[2][i]);
    }
}

void buildPaletteArray(uint8_t arrayType, uint8_t arrayTarget, uint8_t baseValue) {
    //uint8_t newArray[16];
    switch (arrayType) {
        case 0:
            for (int i=0; i<16; i++) {
                palettePatterns[arrayTarget][i] = 255;
            }
            break;
            
        case 1:
            for (int i=0; i<15; i+=2) {
                palettePatterns[arrayTarget][i] = 0;
                palettePatterns[arrayTarget][i+1] = 255;
            }
            break;
        case 2:
            for (int i=0; i<16; i++) {
                palettePatterns[arrayTarget][i] = i*16;
            }
            break;
        case 3:
            for (int i=0; i<16; i++) {
                palettePatterns[arrayTarget][i] = baseValue;
            }
            break;
            
        case 4:
            for (int i=0; i<16; i++) {
                palettePatterns[arrayTarget][i] = sin8(i*16);
            }
            break;
        case 5:
            
            for (int i=0; i<15; i+=2) {
                palettePatterns[arrayTarget][i] = baseValue;
                palettePatterns[arrayTarget][i+1] = (baseValue+60);
            }
            break;
        case 6:
            for (int i=0; i<16; i++) {
                palettePatterns[arrayTarget][i] = sin8((i*64) % 256);
            }
            break;
    }
    
}

void SetupAlternatingPalette(uint8_t hue1, uint8_t hue2, uint8_t sat1, uint8_t sat2) 
{
    targetPalette = CRGBPalette16( 
                      NSFastLED::CHSV( hue1, sat1, 128), 
                      NSFastLED::CHSV( hue2, sat1, 255), 
                      NSFastLED::CHSV( hue1, sat2, 255), 
                      NSFastLED::CHSV( hue2, sat2, 255)); 
}


// This function generates a random palette that's a gradient
// between four different colors.  The first is a dim hue, the second is 
// a bright hue, the third is a bright pastel, and the last is 
// another bright hue.  This gives some visual bright/dark variation
// which is more interesting than just a gradient of different hues.
void SetupRandomPalette()
{
if (count8<1) {
    targetPalette = CRGBPalette16( 
                      NSFastLED::CHSV( random8(), 255, 128), 
                      NSFastLED::CHSV( random8(), 255, 255), 
                      NSFastLED::CHSV( random8(), 128, 255), 
                      NSFastLED::CHSV( random8(), 255, 255)); 
    }
}


void SetupRandomPalette16(uint8_t hueLower, uint8_t hueUpper, uint8_t satLower,  uint8_t satUpper)
{
    uint8_t startingHue = random8(hueLower,hueUpper);
    uint8_t startingSaturation = random8(satLower, satUpper);
    
    if (count8<1) {
        targetPalette[0] =  NSFastLED::CHSV( startingHue, startingSaturation, 255 );
        for(int i=1; i < 15; i++)
        {
            targetPalette[i] =  NSFastLED::CHSV( random8(hueLower,hueUpper), random8(satLower, satUpper), 255 );
        }
        targetPalette[15] = NSFastLED::CHSV( startingHue, startingSaturation, 255 );
    }
}


void SetupCrystalPalette()
{
    uint8_t hueLower = 110;
    uint8_t hueUpper = 220;
    uint8_t satLower = 120;
    uint8_t satUpper = 255;
    
    SetupRandomPalette16(hueLower, hueUpper, satLower, satUpper);
}

void SetupRangedPalette()
{
    SetupRandomPalette16(mainColor[0], secondaryColor[0], mainColor[1], secondaryColor[1]);
}


void SetupSunnyCloudsPalette()
{
  static uint8_t cloudSaturation = 0;
  static uint8_t skySaturation = 230;
  
  if (cloudiness <= 127) {
    cloudSaturation = 255-cloudiness*1.9;
    skySaturation = 230;
  } else {
    cloudSaturation = 0;
    skySaturation = 230-(cloudiness-127)*1.7;
  }
  
  targetPalette = CRGBPalette16( 
                      NSFastLED::CHSV( 160, skySaturation, 255-cloudiness/2 ), 
                      NSFastLED::CHSV( 160, cloudSaturation, 255 ), 
                      NSFastLED::CHSV( 140, skySaturation, 255-cloudiness/2 ), 
                      NSFastLED::CHSV( 160, cloudSaturation, 255 )); 

}

void SetupSunsetPalette()
{
  
  static uint8_t skyBrightness = 255;
  static uint8_t cloudBrightness = 255;
  
  if ( countPalette <= 201) {
    // The sun is setting, but still out
    cloudBrightness = 255;
    skyBrightness = 255- countPalette/2;
  } else {
    // The sun sinks beneath the horizon
    cloudBrightness = 256-( countPalette-200)*4.6;
    skyBrightness = 180-( countPalette-200)*3;
  }
  
  targetPalette = CRGBPalette16( 
                      NSFastLED::CHSV( 160, 255, skyBrightness ), 
                      NSFastLED::CHSV( 245+random8(30),  countPalette, cloudBrightness ), 
                      NSFastLED::CHSV( 160, 255, skyBrightness ), 
                      NSFastLED::CHSV( 245+random8(30),  countPalette, cloudBrightness )); 
  
}

void SetupSunrisePalette()
{
  
  static uint8_t skyBrightness = 0;
  static uint8_t cloudBrightness = 0;
  static uint8_t skySaturation = 255;
  
  if ( countPalette <= 50) {
    // Before the sun rises
    cloudBrightness =  countPalette*4.6;
    skyBrightness =   0;
    skySaturation = 180;
    colorLoop = 0;
    speed =  1;
  } else if ( countPalette <= 100){
    // The Sky is changing colors
    skyBrightness = ( countPalette-50)*4.6;
    cloudBrightness = 230-(( countPalette-50)*1.6);
    skySaturation = 200;
    colorLoop = 1;
    speed =  1;
  } else {
    // Sunrise into day
    cloudBrightness = (( countPalette-10)*1);
    skyBrightness = 240;
    skySaturation = 220;
    speed =  3;
  }
  
  targetPalette = CRGBPalette16( 
                      NSFastLED::CHSV( 160, skySaturation, skyBrightness ), 
                      NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness ), 
                      NSFastLED::CHSV( 160, skySaturation, skyBrightness ), 
                      NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness )); 
    //currentPalette = targetPalette;
  /*
  // 'black out' all 16 palette entries...
  fill_solid( currentPalette, 16, NSFastLED::CHSV( 160, skySaturation, skyBrightness ) );
  
  // 30 degrees on color wheel is about 21.3 out of 256
  currentPalette[0] = NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness );
  currentPalette[1] = NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness );
  
  currentPalette[4] = NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness );
  currentPalette[5] = NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness );
  
  currentPalette[8] = NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness );
  currentPalette[9] = NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness );
  
  currentPalette[12] = NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness );
  currentPalette[13] = NSFastLED::CHSV( 60+random8(10), 256- countPalette, cloudBrightness );
  */
}


void SetupMoonlightPalette()
{
  
  static uint8_t cloudBrightness = 0;
  
  if ( countPalette <= 50) {
    // Darkness before moonlight
    cloudBrightness =  countPalette*3;
  } else if ( countPalette <= 200){
    // Clouds and moonlight
    cloudBrightness = 150;
  } else {
    // Fade out before sunrise
    cloudBrightness = 150-( countPalette-200)*2.6;
  }
  
  targetPalette = CRGBPalette16( 
                      NSFastLED::CHSV( 160, 0, 0 ), 
                      NSFastLED::CHSV( 60+random8(10), 0, cloudBrightness ), 
                      NSFastLED::CHSV( 160, 0, 0 ), 
                      NSFastLED::CHSV( 60+random8(10), 0, cloudBrightness )); 

}


/*

Color Harmony Patterns

We've found these useful for trying out and thinking about different color combinations.
They are based on ideas about which colors may be pleasing in combinations.
Of course, this isn't an exact science, so we want to play with brightness and saturation
and some randomization on hue as well.

*/

void SetupAnalogousPalette()
{
  // Set all 16 palette entries to reference
  fill_solid( targetPalette, 16, NSFastLED::CHSV( countPalette, 255, 127 ) );
  
  // 30 degrees on color wheel is about 21.3 out of 256
  targetPalette[0] = NSFastLED::CHSV( countPalette-21, 180, 255 );
  targetPalette[1] = NSFastLED::CHSV( countPalette,    255, 255 );
  targetPalette[2] = NSFastLED::CHSV( countPalette+21, 180, 255 );
  
  targetPalette[6] = NSFastLED::CHSV( countPalette+21, 255, 255 );
  targetPalette[7] = NSFastLED::CHSV( countPalette,    180, 255 );
  targetPalette[8] = NSFastLED::CHSV( countPalette-21, 255, 255 );
  
  targetPalette[11] = NSFastLED::CHSV( countPalette+21, 180, 255 );
  targetPalette[12] = NSFastLED::CHSV( countPalette-21, 255, 255 );
  targetPalette[13] = NSFastLED::CHSV( countPalette+21, 180, 255 );
  targetPalette[15] = NSFastLED::CHSV( countPalette-21, 180, 255 );
  
}


void SetupComplementaryPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( targetPalette, 16, CRGB::Black );
  
  // 30 degrees on color wheel is about 21.3 out of 256
  targetPalette[0] = NSFastLED::CHSV( countPalette, 255, 255 );
  targetPalette[1] = NSFastLED::CHSV( countPalette, 255, 255 );
  
  targetPalette[4] = NSFastLED::CHSV( countPalette+128, 255, 255 );
  targetPalette[5] = NSFastLED::CHSV( countPalette+128, 255, 255 );
  
  targetPalette[8] = NSFastLED::CHSV( countPalette, 200, 255 );
  targetPalette[9] = NSFastLED::CHSV( countPalette, 200, 255 );
  
  targetPalette[12] = NSFastLED::CHSV( countPalette+128, 200, 255 );
  targetPalette[13] = NSFastLED::CHSV( countPalette+128, 200, 255 );
  targetPalette[15] = NSFastLED::CHSV( countPalette, 255, 255 );
  
}

void SetupSplitComplementaryPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( targetPalette, 16, CRGB::Black );
  
  // 30 degrees on color wheel is about 21.3 out of 256
  targetPalette[0] = NSFastLED::CHSV( countPalette-21, 180, 255 );
  targetPalette[1] = NSFastLED::CHSV( countPalette-21, 255, 255 );
  targetPalette[2] = NSFastLED::CHSV( countPalette-21, 127, 255 );
  
  targetPalette[4] = NSFastLED::CHSV( countPalette+128, 255, 255 );
  
  targetPalette[6] = NSFastLED::CHSV( countPalette+21, 255, 255 );
  targetPalette[7] = NSFastLED::CHSV( countPalette+21, 127, 255 );
  targetPalette[8] = NSFastLED::CHSV( countPalette+21, 255, 255 );
  
  targetPalette[10] = NSFastLED::CHSV( countPalette+128, 180, 255 );
  
  targetPalette[12] = NSFastLED::CHSV( countPalette+21, 180, 255 );
  targetPalette[13] = NSFastLED::CHSV( countPalette-21, 150, 255 );
  targetPalette[14] = NSFastLED::CHSV( countPalette+128, 255, 255 );
  targetPalette[15] = NSFastLED::CHSV( countPalette-21, 180, 255 );
}

void SetupDoubleComplementaryPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( targetPalette, 16, CRGB::Black );
  
  // 30 degrees on color wheel is about 21.3 out of 256
  targetPalette[0] = NSFastLED::CHSV( countPalette, 255, 255 );
  targetPalette[2] = NSFastLED::CHSV( countPalette, 150, 255 );
  
  targetPalette[4] = NSFastLED::CHSV( countPalette+128, 255, 255 );
  targetPalette[6] = NSFastLED::CHSV( countPalette+128, 150, 255 );
  
  targetPalette[8] = NSFastLED::CHSV( countPalette+43, 200, 255 );
  targetPalette[10] = NSFastLED::CHSV( countPalette+43, 150, 255 );
  
  targetPalette[12] = NSFastLED::CHSV( countPalette+171, 200, 255 );
  targetPalette[14] = NSFastLED::CHSV( countPalette+171, 150, 255 );
  targetPalette[15] = NSFastLED::CHSV( countPalette, 255, 255 );
}


void SetupTriadicPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( targetPalette, 16, CRGB::Black );
  
  // A third around the color wheel is 85 out of 256
  targetPalette[0] = NSFastLED::CHSV( countPalette-85, 200, 255 );
  targetPalette[1] = NSFastLED::CHSV( countPalette-85, 255, 255 );
  targetPalette[2] = NSFastLED::CHSV( countPalette-85, 200, 255 );
  
  targetPalette[6] = NSFastLED::CHSV( countPalette+85, 255, 255 );
  targetPalette[7] = NSFastLED::CHSV( countPalette+85, 200, 255 );
  targetPalette[8] = NSFastLED::CHSV( countPalette+85, 255, 255 );
  
  targetPalette[11] = NSFastLED::CHSV( countPalette, 200, 255 );
  targetPalette[12] = NSFastLED::CHSV( countPalette, 255, 255 );
  targetPalette[13] = NSFastLED::CHSV( countPalette, 200, 255 );
  targetPalette[15] = NSFastLED::CHSV( countPalette-85, 200, 255 );
}




void SetupNeoPalette()
{
  targetPalette = CRGBPalette16( 
                      NSFastLED::CHSV( 96, 255, 0), 
                      NSFastLED::CHSV( 96, 255, 255), 
                      NSFastLED::CHSV( 96, 180, 0), 
                      NSFastLED::CHSV( 96, 255, 128)); 
}

void SetupLightCyclePalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( targetPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  targetPalette[0] = NSFastLED::CHSV( 32, 255, 255);
  targetPalette[4] = NSFastLED::CHSV( 132, 255, 255);
  targetPalette[8] = NSFastLED::CHSV( 32, 230, 255);
  targetPalette[12] = NSFastLED::CHSV( 132, 230, 255);
}


void SetupBlackPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( targetPalette, 16, CRGB::Black);
}


// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( targetPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  targetPalette[0] = CRGB::White;
  targetPalette[4] = CRGB::White;
  targetPalette[8] = CRGB::White;
  targetPalette[12] = CRGB::White;

}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
  CRGB purple = NSFastLED::CHSV( HUE_PURPLE, 255, 255);
  CRGB green  = NSFastLED::CHSV( HUE_GREEN, 255, 255);
  CRGB black  = CRGB::Black;
  
  targetPalette = CRGBPalette16( 
    green,  green,  black,  black,
    purple, purple, black,  black,
    green,  green,  black,  black,
    purple, purple, black,  black );
}




/*

Weather Patterns

The goal here is to illuminate the clouds in the foreground
and the sky in the background differently thoughout the day.

There are 24*60=1440 minutes per 24hr day,
so we animate a time lapse with 0.25 minutes per frame
for a total of 5760 frames. That's 96 seconds at 60fps.
There are 240 frames per hour in 4 seconds at 60fps.

Minutes 0 to 600 - Sunrise
As the sun rises, the sky is dark and the clouds illuminate with yellow
The sky fades in to pale blue and then deepens in saturation 
as the clouds fade to dim and then bright white

Minutes 601 to 2400 - Daytime
The day is represented with white clouds on a deep blue sky,
but as the day gets cloudier, the sky darkens into a thunderstorm.
Thuderstorms are rendered the same during the day as they are at night.

Minutes 2401 to 3000 - Sunset
If the day is only cloudy and not storming,
the sun turns the clouds pink and purple as it sets
and the sky deepens and slowly darkens.

Minutes 3001 to 5760 - Moonlight
The clouds are dimly lit by the moon and the sky is black

*/



void AnimateWeatherPalette()
{
  //static uint8_t cloudSaturation = 0;
  //static uint8_t skySaturation = 230;
  
  // First, test the cloudiness.
  // If it's over 192, then animate a thunderstorm
  
  if (cloudiness >= 192) {
    // There is a thunderstorm of 0-63 in severity
    // Severity causes the clouds to darken
    // and the lightning to strike more frequently
    
    
  } else {
    // There are 0-15 clouds evenly dispersed across the sky
    
    // --- Sunrise ---
    // Black out the sky before sunrise
    //if (countMinutes ==   0) { currentPalette = CRGBPalette16( CRGB::Black );                      }
    // Golden sunrise                              clouds            sky                 cloudRandomness
    if (countMinutes ==   3) { SetupWeatherPalette(30,230,140, 40,230,120,  40);    }
    // Bring down the sky and keep brightening the clouds
    if (countMinutes == 200) { SetupWeatherPalette(30,230,200, 40,230,  0,  40);    }
    if (countMinutes == 220) { SetupWeatherPalette(40,230,200, 160,230,  0,  30);    }
    // Bring up the sky in blue as the clouds fade back out
    if (countMinutes == 300) { SetupWeatherPalette(50,230, 90,160,230, 90,  30);    }
    // Keep bringing up the sky and clouds and bring the clouds to white
    if (countMinutes == 400) { SetupWeatherPalette(50,  0,192,160,230,192,  30);    }
    if (countMinutes == 500) { SetupWeatherPalette(90,  0,192,160,230,200,  30);    }
    if (countMinutes == 600) { SetupWeatherPalette(245,  0,220,160,230,200,  30);    }
    
    // --- Sunset ---
    if (countMinutes ==2400) { SetupWeatherPalette(245, 90,220,160,240,190,  30);    }
    if (countMinutes ==2500) { SetupWeatherPalette(245,120,200,160,240,180,  30);    }
    if (countMinutes ==2700) { SetupWeatherPalette(245,230,150,160,240,150,  30);    }
    if (countMinutes ==2900) { SetupWeatherPalette(245,230,  0,160,240,  0,  30);    }
    
    // --- Moonlight ---
    if (countMinutes ==3100) { SetupWeatherPalette(245,  0,  0,160,  0,  0,  30);    }
    if (countMinutes ==3300) { SetupWeatherPalette(245,  0,120,160,  0, 50,  30);    }
    
    if (countMinutes ==5600) { SetupWeatherPalette(245,  0,0,160,  0, 0,  30);    }
  }
  
  // Slow the rate that the palettes are blending, but let them blend evenly
  if ((countMinutes / MINUTES_SPEED) % 6 == 0) { nblendPaletteTowardPalette( currentPalette, targetPalette, maxChanges);}
  
}

//void SetupWeatherPalette(NSFastLED::CHSV cloudColor, NSFastLED::CHSV skyColor, uint8_t cloudRandomness)
void SetupWeatherPalette(uint8_t cloudColorHue, uint8_t cloudColorSaturation, uint8_t cloudColorValue, uint8_t skyColorHue, uint8_t skyColorSaturation, uint8_t skyColorValue, uint8_t cloudRandomness)
{
  
  
  for (int i = 0; i < 16; i++) {
    if (i >= cloudiness / 16) {
      // Apply sky color
      targetPalette[i] = NSFastLED::CHSV( skyColorHue, skyColorSaturation, skyColorValue);;
    } else {
      // Apply cloud color and some randomization
      targetPalette[i] = NSFastLED::CHSV( cloudColorHue, cloudColorSaturation, cloudColorValue);
      
    }
  }
  
} 

