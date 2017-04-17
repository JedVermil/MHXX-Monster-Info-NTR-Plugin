# MHXX-Monster-Info-NTR-Plugin

This NTR overlay plugin displays monster info (such as HP) during quests on Monster Hunter Double Cross (MHXX).

## How to use

Install NTR 3.4 preview 4 if you haven't already; BootNTR selector is highly recommended as it's a nice all-in-one package. Older versions of NTR are not supported due to the necessity of overlay functionality. Put the release folder (0004000000197100) in the /plugins/ folder of your memory card. The plugin should automatically activate once you have NTR up and running.

Once a mission loads you will see all monsters listed on the bottom of the screen. Only large monsters will be listed at all times, while the small monsters will only be listed if they are in the current area. Color coding of HP values are:
- Green: >30% health
- Yellow: 30% ~ 20% health
- Red: 20% ~ 0% health

## Sample Display

![Alt text](/example.jpg?raw=true "Optional Title")

1. Monster HP (always white)
2. Monster HP
3. Part 1 HP
4. Part 2 HP

HP bars will change color depending on what percentage is left. Only large monsters will have part HP displays. The relative lengths of the part HP bars indicate their relative values; in other words, longer bars mean higher HP. Note that not all parts will be displayed; for most monsters only the tail HP will show up. This limitation will not be solved until individualized profiles for each monster have been created, as the rules governing how each part counts as being broken can vary quite a bit.

## Notes

Currently only Monster Hunter Double Cross (MHXX) v1.1 is supported; previous games (MHX/MHGenerations) are not supported. Other versions of the game may be supported but not guaranteed.
