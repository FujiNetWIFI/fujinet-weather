# fujinet-weather
The canonical weather application for Atari, ADAM, C64, Coco, and Apple2

June 2024 - This codebase is now in its own repo. It has been pulled from fujinet-apps and moved here.
Please create all forks and enhancements to this application here via forks and PRs.

-----------------------


# Update Notes

### ADAM
  June 24 - This version has the following features
  1) Option feature the change polling frequency
  2) Only asks for weather no more than once every 10 minutes
  3) Calls location by ip only once instead of everytime the application is run
  4) You can manually enter the latitude and logitude if desired


### Apple2
 - June 2024
   - move build/makefile over to fenrocks makefile system
   - adjusted BSS to fit in space allocated


# Building on Platforms

### Atari
- need the Mad-Pascal (MP) 32-bit Turbo Pascal compiler - https://mads.atari8.info/doc/en/index.html
- no more info currently


### ADAM
- need zcc compiler for ADAM.
  - https://github.com/z88dk/z88dk/wiki



### Apple2
- cc65 & fn-libs (pulled automatically) are used to build the Apple2

  - git clone https://github.com/cc65/cc65.git
  - make clean && make PREFIX=/usr/local && sudo make install PREFIX=/usr/local
  - Verify with this command:
    - cl65 --print-target-path


### c64
- cc65 is used to build for the C64
  - See Apple2 above for cc65 installations


### Coco
- Color Computer builds with CMOC - CMOC is a 6809-generating cross-compiler for a large subset of the C language
  - http://perso.b2b2c.ca/~sarrazip/dev/cmoc.html
    - download lwtools, make, make install
      - http://www.lwtools.ca/releases/lwtools/lwtools-4.22.tar.gz
    - download cmoc, make, make install
      - http://perso.b2b2c.ca/~sarrazip/dev/cmoc-0.1.86.tar.gz
  - The CoCo 3 icon header is checked in, so the build does not require the source PNGs. To regenerate it, set `MM_ICONS_DIR` to the directory containing the Meteomatics `wsymbol_*.png` files.



### VIC-20
- cc65 is used to build for the VIC
  - See Apple2 above for cc65 installations


# Usage and Application Notes

### Apple 2
- Apple2 builds fine but it requires a API key from OpenWeatherMap
  - https://home.openweathermap.org
  - You need to setup an account, request on-by-one and get an API Key
  - You then must upgrade that key to be "Base Plan" - which is free for first 1000 calls
    - you will need to register your name a credit card to get the plan

 This isn't optimal but it's what required right now in June 2024, other API plans will be explored.

  - Put the contents of the KEY (32 bytes long) into a file named '56000101.key'.
  - Save the file in the /FujiNet directory of your Fujinet SD card.
    - full path would be: SD/FujiNet/56000101.key
    - Note capitalization


 ## Other Platforms

 Work as expected...
