# Shadows of the Damned: Hella Remastered Fix
[![Patreon-Button](https://raw.githubusercontent.com/Lyall/SotDFix/refs/heads/master/.github/Patreon-Button.png)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/SotDFix/total.svg)](https://github.com/Lyall/SotDFix/releases)

This is a fix for Shadows of the Damned: Hella Remastered that adds ultrawide/narrower support, allows adjusting gameplay FOV and more.

## Features
### General
- Remove 60FPS cap (WIP, see [issues](#Known-Issues)).
- Adjust gameplay FOV.
- Increase LOD distance.
 
### Ultrawide/Narrower
- Support for any resolution/aspect ratio.
- Fix cropped FOV at when using an ultrawide display.
- Fix stretched HUD.

## Installation
- Grab the latest release of SotDFix from [here.](https://github.com/Lyall/SotDFix/releases)
- Extract the contents of the release zip in to the the game folder. e.g. ("**steamapps\common\ShadowsOfTheDamned**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="winmm=n,b" %command%` to the launch options.

## Configuration
- See **SotDFix.ini** to adjust settings.

## Known Issues
Please report any issues you see.

- Uncapping the framerate can cause some issues such as desynchronisation of audio. ([#8](https://github.com/Lyall/SotDFix/issues/8))

## Screenshots
![ezgif-2-6f2171aee5](https://github.com/user-attachments/assets/1c07e4f5-e3b1-4feb-b4ba-346b959b5e6a)

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
