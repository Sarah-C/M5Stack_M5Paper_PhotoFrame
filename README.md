# M5Stack_M5Paper_PhotoFrame
This is a dithered photo viewer.

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

The difference when using a **dithered** image viewer:

These two images are using a standard jpeg decoder - that doesn't dither pixels.
![20210819_081528](https://user-images.githubusercontent.com/1586332/130035122-2909c1e6-0eb7-444d-9f9f-e5629662ae6b.jpg)

![20210819_081542](https://user-images.githubusercontent.com/1586332/130035139-02f36a2e-da2e-4cff-9113-6c4038fc2ea2.jpg)


Here's the dithered verisons - 

![20210819_081528](https://user-images.githubusercontent.com/1586332/130035277-de07fb54-a117-44fa-94c3-5f31b8104e71.jpg)

![20210819_081826](https://user-images.githubusercontent.com/1586332/130035312-7e855e04-ea1b-438b-9f00-7ac5d7603738.jpg)

