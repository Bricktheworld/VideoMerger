FFMpeg required: version 4.2.4-1 (May work on other versions but have not tested)

Put VideoMerger executable in the directory of all of the video files

How it works:

1. Detects all of the files by file type
2. Calculates the dimensions of each video using the specified rows and columns
3. Orders all of the files alphabetically
4. Crops video files and puts them in "cropped" folder
5. After cropping all of the videos creates a compiled video using the cropped videos and the specified dimensions

What to do if 1 video doesn't work:

1. Create a separate directoy with the specific video and the VideoMerger executable
2. Run video merger on that 1 file
3. Specify the rows and columns that was used previously (Do not just do 1x1, make it the actual number of rows and columns)
4. Run ffmpeg
5. Copy the output of the video in the "cropped" folder to the old "cropped" with all of the other videos. Basically copy the output of the individually run VideoMerger on the individual file and copy it to the cropped folder
6. Rerun the VideoMerger in the directory with all of the videos and IMPORTANTLY, type "n" when it asks if you would like to delete the cropped directory and try again, otherwise you will have to go through the entire cropping process over again

NOTE this can also work for multiple videos if only a couple of the videos didn't work and you only want to recrop those ones

Also if you are just trying to line up the video, you can also just import the cropped video into a video editor and line it up, then put it back and rerun the merger, IMPORTANTLY selecting "n" when asked if you want to delete the cropped directory and restart.

It is recommended that you back up the "cropped" directory many times so as not to lose progress in case you accidentally select y when asked if you want to delete the cropped dir

If you need to manually crop a video, the dimensions for each video that are going to be cropped to are always outputted before running

How to compile:
g++ VideoMerger.cpp XYVector.cpp -o VideoMerger
