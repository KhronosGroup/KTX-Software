<!-- Copyright 2025 Mark Callow -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

SDL_gesture.h
-------------

The Gesture API was removed from SDL3. As a migration path they provided an equivalent single-header library `SDL_gesture.h` that can be dropped into an SDL3-based project.

They do not make formal releases of this code; they say "just grab the latest and drop it into your project!"

The origin of this file is fork https://github.com/MarkCallow/SDL_gesture.git whose upstream is
https://github.com/libsdl-org/SDL_gesture. It includes modifications for robustness to prevent production of spurious GESTURE\_MULTIGESTURE events. 
