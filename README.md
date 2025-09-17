# ShimmerVerb (JUCE) – No-GUI (Generic GUI)

A cinematic shimmer reverb using **ZitaRev core** + **granular pitch-shift layers** (octave, fifth, etc).  
This repository contains a minimal, no-GUI Audio Plugin (VST3 + Standalone).  
Tested on **Windows 10/11 + Visual Studio 2022**.  
Author: **Danial Kooshki** | [www.danialkooshki.com](http://www.danialkooshki.com)

---

##  Features
- Zita-style reverb core (parameters: decay, low RT60, HF damp)
- Shimmer feedback loop with **multi-layer granular pitch shift** (configurable semitone layers)
- Loop tone shaping (HPF/LPF)
- Clean dry/wet mixing outside the reverb
- Builds as **VST3** and **Standalone**

---

## ⚙️ Requirements

- [JUCE](https://juce.com/get-juce) (tested with JUCE 7.x)
- [Visual Studio 2022](https://visualstudio.microsoft.com/) (Desktop development with C++)
- [Git](https://git-scm.com/downloads)
- [Faust](https://faust.grame.fr/) (needed for ZitaRev, install before building)

---

## 📦 Installation & Setup

1. **Clone this repo:**
   ```bash
   git clone https://github.com/<your-username>/ShimmerVerb.git
   cd ShimmerVerb
   ```

2. **Install JUCE**  
   - Download JUCE or build from source.  
   - Add JUCE to your system path or remember the folder.

3. **Install Faust (required for ZitaRev)**  
   - Download Faust binaries from [Faust releases](https://github.com/grame-cncm/faust/releases).  
   - Extract to a folder (e.g. `C:\Faust`).  
   - Add the Faust `bin` folder to your system PATH.  
   - Ensure you can run `faust --version` in your terminal.  
   - In Projucer, add Faust include directory to **Extra Header Search Paths** so `#include "ZitaRev.cpp"` works.

---

## 🛠️ Build (Windows)

### Using Projucer
1. Open `ShimmerVerb.jucer` with Projucer.  
2. Set **JUCE path** correctly in *Projucer → Global Paths*.  
3. Enable **Plugin Formats** you need (VST3, Standalone).  
4. Save and export as *Visual Studio 2022 solution*.  

### Build with MSBuild
```bash
cd Builds\VisualStudio2022
msbuild ShimmerVerb.sln /t:Build /p:Configuration=Release /p:Platform=x64
```

Output will be in:
```
Builds\VisualStudio2022\x64\Release\VST3\ShimmerVerb.vst3
Builds\VisualStudio2022\x64\Release\Standalone Plugin\ShimmerVerb.exe
```

---

##  Usage
- Copy `ShimmerVerb.vst3` to your DAW’s VST3 folder:
  ```
  C:\Program Files\Common Files\VST3\
  ```
- Open your DAW and load **ShimmerVerb**.  
- For testing without a DAW, run the standalone build:  
  ```
  ShimmerVerb.exe
  ```

---

## 📄 License

MIT License  

Copyright (c) 2025 **Danial Kooshki**

Permission is hereby granted, free of charge, to any person obtaining a copy  
of this software and associated documentation files (the "Software"), to deal  
in the Software without restriction, including without limitation the rights  
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
copies of the Software, and to permit persons to whom the Software is  
furnished to do so, subject to the following conditions:  

The above copyright notice and this permission notice shall be included in  
all copies or substantial portions of the Software.  

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING  
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS  
IN THE SOFTWARE.

---

## 🌐 Author
**Danial Kooshki**  
[www.danialkooshki.com](http://www.danialkooshki.com)
