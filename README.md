# RL
Roguelike work

2/6/2015
This is kinda a mess. 
Using <a href="http://tapiov.net/rlutil/">rlutil</a> (as per <a href="http://tapiov.net/rlutil/docs/License.txt">the license</a>) as a starting point for primitive console put & color functions - also because it is drop dead easy using a header include - I have so far FAILED to figure out how to link against a more robust library (libtcod for example).

- rlutil uses 1,1 as the top-left index - I prefer 0,0
- rlutil uses column, row notation - of course, I prefer row, column

So - I have been using the CodeLite IDE environment and MinGW as the compiler. I have made no attempt to make the code portable - it is specifically a Windows console program.
