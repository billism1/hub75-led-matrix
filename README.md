# 64x64 LED matrix HUB75 Example

This is a repo I'm using to house projects that uses the 64x64 LED matrix HUB75 panel I bought from Ali Express in late 2023. The model number on the back of the panel is "P3(2121)64x64-32S-M5.1". All of the pin in/out documentation I found online for device indicated that there should be an A, B, C, D, and E pin, where as the panel I have has the pin labelled, "GND" where the D pin should be. Using a multimeter and testing for continuity, this pin doesn't appear to be connected to the other "GND" pins. Folling the guid linked below, it appears fully funcitonal, though. I'm leaving this note here in case it helps anyone else trying to figure this out.

To drive the panel, I am using the library from the repo: [mrfaptastic/ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA). This is also the same library used with [Brian Lough's ESP32-Trinity open source ESP32 board for controlling HUB75 matrix panels](https://github.com/witnessmenow/ESP32-Trinity/).

## The projects

The following projects use parameters that work with the above mentioned specific panel.

- [Sand Matrix](./src/sand)
  - Code ported from [my other sand matrix project](https://github.com/billism1/sand-matrix).
  
- [Sand Matrix with Accelerometer](./src/sand-accel)
  - Adafruit [LIS3DH](https://learn.adafruit.com/adafruit-lis3dh-triple-axis-accelerometer-breakout) accelerometer added to the [Sand Matrix](./src/sand) project.

- [Pixel Time](./src/AuroraDemo)
  - Copied from: [2dom/PxMatrix pixeltime example](https://github.com/2dom/PxMatrix/tree/master/examples/pixeltime) and modified to use with the [ESP32-HUB75-MatrixPanel-DMA library](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA).

- [Aurora Demo](./src/PixelTime)
  - Copied from: [2dom/PxMatrix Aurora_Demo example](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA).
