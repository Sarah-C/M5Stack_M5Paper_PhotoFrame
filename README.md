# M5Stack M5Paper: Photo viewer
This is a dithered photo viewer for the M5Stack M5Paper device.                      
It's capable of drawing 960x540 resolution pictures in 16 shades of grey.            

Until now I've not found a photo-viewer program for it that also dither's away the banding that happens when gentle gradients are shown. So I decided to find the best program and libraries I could find and combine them into the currently best M5Paper photo viewer available!

**Now with some scaling/centering.**
You need to put your collection of images in the root of an SD card for the M5Paper.                 
Small images are centered but NOT enlarged, and images larger than 960x540 will be shrunk by a half/ a quarter / or an eigth - whichever is first to fit the screen. 
So for best results it's best to get a collection of 960 x 540 images to fill the entire screen.                       

The toggle wheel on the M5Paper can control the program:         
Toggle left (anticlockwise) : Previous image.             
Toggle right (clockwise)    : Next image. (Next image happens naturally every 2 minutes)             
Press the toggle in         : Turn the M5Paper off / on. (When turning off, the file text changes from "-blah- .jpg" to "-blah- .Off", it's subtle!              
Note: The M5Paper DOES NOT turn off when it's plugged into the USB socket! So you'll get it to say ".Off".. but it'll carry on anyway. You need to unplug it first before turning off.          
           
There's an images folder included for you to privately test the program - please don't use them for anything more! Their copyrights are respectively their owners.             
     
Thanks to bitbank2 for the really fast and dithered JPEG output:            
  https://github.com/bitbank2/JPEGDEC            
  
Thanks to wizche for the photo-viewer:               
  https://github.com/wizche/flip-pics           
              
         
              
IMPORTANT!             
In your documents folder, after you've downloaded the library JPEGDEC,             
open the file : Documents\Arduino\libraries\JPEGDEC\src\JPEGDEC.h                 
Change line 49, to read:            
         
#define MAX_BUFFERED_PIXELS 8192          
           
If you don't - many pictures will end up with horizontal lines on them due to the 960 pixel e-ink paper screen width.           
(I should have set up the pictures display vertically, then this change might not have been needed, being 540 pixels wide.)         
          
The difference when using a **dithered** image viewer:          
         
Here's the wonderful quality of the dithered verisons this program uses -        
           
![20210819_094344](https://user-images.githubusercontent.com/1586332/130038142-1dad8b62-3d64-4a0a-bab9-514a60af25c2.jpg)             

See how well a WORST CASE bad banding image is shown? You can't see the bands at all:             

![20210819_081845](https://user-images.githubusercontent.com/1586332/130035486-fa2e13ce-0954-46ec-9f05-3cc708a0be89.jpg)            
            
Here's another worst-case example of banding being really well drawn by the ditherer, rooming all the banding:                

![20210819_081826](https://user-images.githubusercontent.com/1586332/130035501-c15486f1-0b2a-4841-8129-4ec7ead69391.jpg)       

       
These two images are using a standard jpeg decoder - that **doesn't** dither pixels. 
Note the terrible banding that's obvious on the gently brightening skies in the background?
          
![20210819_081528](https://user-images.githubusercontent.com/1586332/130035520-a7000128-d04a-40ee-906b-3a426f5204e2.jpg)           
           
These examples are showing how bad the banding is when images are drawn to E-ink panels without dithering the pixels...

![20210819_081542](https://user-images.githubusercontent.com/1586332/130035533-4de85768-bf3b-4ed9-ad6a-7306b4f1bb41.jpg)          
     
