# Simple Trio - Pebble Watchface

## Project Overview
This is a **Pebble smartwatch analog watchface** called "Simple Trio" by Eduardo Chiaro. It's a fork/copy of another project ("Simple Enough" based on README) that has been customized.

## Technical Stack
- **Language**: C (Pebble SDK 3)
- **Build System**: wscript (Waf build tool)
- **Configuration**: JavaScript with Clay framework for settings UI
- **Target Platforms**: All Pebble models (Aplite, Basalt, Chalk, Diorite, Emery, Flint)

## Core Components

### 1. Main C Code (`src/c/simple-trio.c`)
**Functionality**:
- Analog clock face with hour and minute hands
- Three radial lines at 10, 2, and 6 o'clock positions
- PDC vector images of numbers "10", "2", and "6" at their respective positions
- Small dots (2px) at remaining hour positions (6px from edge)
- Red clock hands with tails extending backward
- White center circle with red border
- Inverted color scheme option

**Key Features**:
- Uses Pebble's GDrawCommandImage for vector graphics (numbers 10, 2, 6)
- Responsive to screen shape (rectangular vs round via `PBL_IF_RECT_ELSE` and `#ifdef PBL_ROUND`)
- Color-aware (handles both B&W and color displays with enhanced logic)
- Smart positioning: numbers positioned 6px from edge along their respective lines
- Dots positioned 6px from edge on rectangular screens (proper edge detection)
- Persistent settings storage
- Real-time updates via tick timer (minute precision)

**Drawing Order**:
1. Background fill
2. Three radial lines at 10, 2, 6 o'clock (light gray, inverts properly on B&W)
3. Small dots at other hour positions (accent color)
4. Number 10 with background (top-left area)
5. Number 2 with background (top-right area)
6. Number 6 with background (bottom center, 12px from border)
7. Hour hand (shorter, red on color / accent on B&W)
8. Minute hand (longer, red on color / accent on B&W)
9. Center circle (white with red border)

### 2. Configuration (`src/pkjs/config.js` & `index.js`)
- Uses Clay framework for settings UI
- Single setting: "Invert Colors" toggle
- Settings persist across watchface restarts
- Bidirectional communication with watch via AppMessage

### 3. Resources
- `6-black.pdc` & `6-white.pdc`: Vector images of the number 6 (for different themes)
- `2-black.pdc` & `2-white.pdc`: Vector images of the number 2 (for different themes)
- `10-black.pdc` & `10-white.pdc`: Vector images of the number 10 (for different themes)
- Menu icon (PNG)
- Assets folder with screenshots

## Current Design Characteristics

### Visual Layout (Rectangular Screens)
- **Radial Lines**: 3 lines at 10, 2, and 6 o'clock positions
- **Hour Dots**: 2px diameter dots at remaining positions, 6px from screen edge
- **Number 10**: Positioned 6px from top-left edge along 10 o'clock line
- **Number 2**: Positioned 6px from top-right edge along 2 o'clock line
- **Number 6**: Centered horizontally, 12px from bottom edge
- **Background Boxes**: Tight-fitting rounded rectangles behind each number
- **Hands**: 
  - Hour: Extends to (screen_width/2 - 28)px from center
  - Minute: Extends to (screen_width/2 - 8)px from center
  - Both have 16px tails on opposite side

### Visual Layout (Round Screens - Chalk)
- **Hour Dots**: 2px diameter, radius-based positioning
- **Numbers 10, 2**: Radius-based positioning, 6px from edge
- Hour hand: Extends to (screen_width/2 - 44)px
- Minute hand: Extends to (screen_width/2 - 22)px
- Other measurements adjust proportionally

### Color Schemes
**Normal Mode**:
- Background: White
- Lines: Light Gray (color screens), Black (B&W screens)
- Accents/Dots: Black
- Hands: Red (color screens), Black (B&W screens)

**Inverted Mode**:
- Background: Black
- Lines: Dark Gray (color screens), White (B&W screens)
- Accents/Dots: White
- Hands: Red (color screens), White (B&W screens)

## Code Quality Notes
- Clean, well-structured C code
- Proper memory management (create/destroy for layers and images)
- Efficient drawing with update callback pattern
- Good use of Pebble SDK macros for cross-platform compatibility
- Settings persistence implemented correctly

## Dependencies
- Pebble SDK 3
- @rebble/clay ^1.0.6 (for configuration UI)

## Build & Deploy
Uses Pebble SDK build system with wscript configuration file.
