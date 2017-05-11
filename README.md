# MHXX-Monster-Info-NTR-Plugin

This NTR overlay plugin displays monster info (such as HP) during quests on Monster Hunter Double Cross (MHXX).

## How to use

Install NTR 3.4 preview 6 if you haven't already; BootNTR selector is highly recommended as it's a nice all-in-one package. Older versions of NTR are not supported due to the necessity of overlay functionality. Put the release folder (0004000000197100) in the /plugins/ folder of your memory card. The plugin should automatically activate once you have NTR up and running. If your system is correctly configured you should see a message on the bottom screen that says "MHXX Overlay Plugin Active" just after loading the game and the screen turns black.

![menu](/menu.png?raw=true "menu")

You can press L + SELECT at anytime to bring up the configuration menu to adjust options. Use the dpad to navigate the menu: left/right toggles the option, while up/down changes the option selection. Press B to exit out of the menu.

Once a mission loads you will see all monsters listed on the bottom of the screen. Only large monsters will be listed at all times, while the small monsters will only be listed if they are in the current area (if configured to be displayed in the settings menu). From left to right the displayed information are: 1) HP 2) HP/part bar 3) poison 4) paralysis 5) sleep. You can also change to display percentages instead of raw numbers. As the HP decreases, the bars shown will change color according to the percentage remaining:
- Green: >30% health
- Orange: 20% ~ 30% health
- Red: 0% ~ 20% health

![legend](/special%20stats%20english.png?raw=true "legend")
![legend2](/special%20stats%20japanese.png?raw=true "legend2")

HP bars are separated by a horizontal divide to segment a top portion and a bottom portion. The top portion shows the main HP, while the bottom portion shows part HPs relative to each other; small monsters do not have the bottom portion. The part HP is a work-in-progress and not only that not all breakable parts are shown, but some of the shown parts don't take damage; this is because the game uses different rules for different parts, and these rules change depending on the monster in question. 

There are 4 locations possible for the display: top screen top-right, top screen bottom-left, bottom screen top-left, and bottom screen bottom-left.

![4locations](/4locations.png?raw=true "4locations")

## Notes

Currently only Monster Hunter Double Cross (MHXX) v1.1 is supported; previous games (MHX/MHGenerations) are not supported. Other versions of the game may be supported but not guaranteed.

If you update the game version to something newer you might find that the monster info display no longer shows anything. If so, try going to the settings menu and run the option "Search for monster list" (by pressing left or right). If everything goes well it should display "SUCCESS" and fix the problem. You need only do this once and the new location will be saved to your config file.

## Special Thanks

This project uses the Misaki font: [github](https://github.com/emutyworks/8x8DotJPFont)
This project uses Kiranico's MHXX database values: [mhxx.kiranico.com](https://mhxx.kiranico.com/)
