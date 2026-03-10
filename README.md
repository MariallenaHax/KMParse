# KMParse
A Very WIP Modern Mario Kart Wii KMP Editor based on Lorenzi's KMP Editor.

## Features
- Key Guide is almost the same as Lorenzi's KMP Editor, which makes it easy to switch from one.
- More Custom AREA Settings! It's supports Conditional out of bounds and BlueLeopard's "Extended KMP Areas" Custom Codes.
- Direct szs editing and writing! (of course its supports open kmp file on working folder)

## Key Guides
W A S D: Move Camera.
Q E: Down Up Camera.
F1: Toggle fog.
F2: Toggle posteffects.
F3: Toggle animations.
F4: Toggle brres/kcl rendering.
F5: Toggle view.
Ctrl + A: Select All.
Ctrl + Z: Undo.
Ctrl + Shift + Z: Redo.
Ctrl + S: Save.
Ctrl + Shift + S: Save As.
Alt + click: Add point.
Alt + click point: duplicate point.
hold Ctrl + click point: multiselect.
Del: delete point.

## To do
- Add Error Check Features: it is similar to "wszst check".
- Add GOBJ, POTI, CKPT, ITPH and ENPH editing supports.
- So many bug fixes.
- Preview Intro Camera Features.

## Build
1. Git clone it repositry
2. create out folder and "cmake .."
3.  "make"

## Credits
- [Noclip](https://github.com/magcius/noclip.website/tree/main): Based on brres renderer and font renderer.
- [StarForge](https://github.com/Astral-C/StarForge/tree/master): Based on Editor Codes.
- [gctoolsplusplus](https://github.com/Astral-C/gctoolsplusplus): The source code includes a modified version of this.
- [Lorenzi's KMP Editor](https://github.com/hlorenzi/kmp-editor): Based on Point Renderer and KMP parser.
