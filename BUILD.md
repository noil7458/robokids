# SPIKE Pi — Build Instructions

## Expected Folder Structure

```
spike-pi/
├── electron/
│   ├── main.js
│   └── preload.js
├── styles/
├── blocks/
├── blockly/
├── generators/
├── serial/
├── ui/
├── assets/
│   ├── icon.png     (512x512 recommended)
│   ├── icon.ico     (Windows)
│   └── icon.icns    (Mac)
├── index.html
├── main.js          (your app's main.js)
└── package.json
```

## 1. Install Dependencies

```bash
npm install
```

## 2. Test in Development (without compiling)

```bash
npm start
```

## 3. Compile Executables

### Windows Only (.exe installer)
```bash
npm run dist:win
```

### Mac Only (.dmg)
```bash
npm run dist:mac
```

### Linux Only (.AppImage)
```bash
npm run dist:linux
```

### All Three at Once
```bash
npm run dist:all
```

The executables are generated in the `dist/` folder.

---

## Important Notes

### Icon
You need to create three icon formats:
- `assets/icon.png` — 512×512px PNG (used on Linux and as base)
- `assets/icon.ico` — Windows format (you can convert at https://icoconvert.com)
- `assets/icon.icns` — Mac format (on Mac: `iconutil`, on Windows: use https://cloudconvert.com)

If you don't have icons yet, you can temporarily omit the icon lines in package.json.

### Cross-compilation
- Building for Windows from Mac/Linux requires Wine or must be done on Windows
- Building for Mac from Windows/Linux is NOT possible without a Mac (Apple restriction)
- Linux AppImage can be compiled from any system

### serialport Module
electron-builder automatically rebuilds native modules (like serialport)
for the correct target. If you get native module errors, run:
```bash
npx electron-rebuild
```

### Difference with Web Version
- In Electron, a port selector appears when connecting (if multiple USB ports are available)
- If only one USB port is connected, it connects automatically
- Does not require Chrome — it is a standalone executable
- WebSerial still works if you open index.html in Chrome directly
