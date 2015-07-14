# SparkCoreLEDcontroller

This code is designed to run on the Spark Core using the FastLED library (which can be imported using the build.particle.io libraries tool).

The FastLED library should be the only prerequisite. The code is written to drive WS2812b LED strips but can be changed to drive any of the other kinds of LED strips that the FastLED library can run by changing the LEDS.addLeds mapping in the setup() function.

You may need to use a logic level translator chip to bring the 3.3v signal from the Spark Core up to the 5v required for many LED strips.
