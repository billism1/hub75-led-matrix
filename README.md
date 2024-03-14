# 64x64 LED matrix HUB75 Example

This is project that uses the 64x64 LED matrix HUB75 panel I bought from Ali Express in late 2023. The model number on the back of the panel is "P3(2121)64x64-32S-M5.1". All of the pin in/out documentation I found online for device indicated that there should be an A, B, C, D, and E pin, where as the panel I have has the pin labelled, "GND" where the D pin should be. Using a multimeter and testing for continuity, this pin doesn't appear to be connected to the other "GND" pins. Folling the guid linked below, it appears fully funcitonal, though. I'm leaving this note here in case it helps anyone else trying to figure this out.

Using the library and guidance from the [2dom/PxMatrix repo](https://github.com/2dom/PxMatrix), I had it working in little time.
