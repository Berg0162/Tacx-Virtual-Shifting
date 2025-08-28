# Tacx-Virtual-Shifting

**Virtual Shifting (VS) for Legacy Smart Tacx trainers that are deprived of the Tacx VS-enabling firmware update.**

## 🚴 What is Virtual Shifting (VS)?
Virtual Shifting lets you “change gears” on your smart trainer without touching your bike’s drivetrain.  
Instead of physically moving your chain across cogs, software simulates different gear ratios and adjusts trainer resistance accordingly.  

This feature is standard on newer Tacx trainers — but older models never received the firmware update. That’s where this project comes in.  

**Goal:** Bring VS to legacy Tacx trainers so that cyclists can enjoy the benefits of shifting *without needing to buy new hardware*.  

## 🌟 Why this project?
- Many Tacx smart trainers are still great but lack VS.  
- Garmin/Tacx only enabled VS on selected newer models.  
- With a bit of software magic, we can unlock a similar experience.  
- This repo provides **an Arduino library + concise examples** so you can try VS yourself.  

## 📦 What you’ll find here
- **`/src`** → the C++ library (building blocks for VS).  
- **`/examples`** → ready-to-run demos showing how to connect and shift.  
- **`/docs`** → background info on Tacx protocols, VS concepts, and troubleshooting.  

## 🛠 Who is this for?
This project is written with **novice programmers who are also cyclists** in mind.  
If you:  
- Have a Tacx smart trainer,  
- Are curious about VS,  
- Know a little Arduino programming (or want to learn),  
then this repo is for you. 🚀  

You don’t need to hack the internals — just upload the example sketches and start experimenting.  

## 📚 Dependencies

This library relies on the following Arduino libraries:

NimBLE-Arduino (v2.x)
 → for Bluetooth Low Energy communication.

Other supporting Arduino libraries (listed in library.properties / platformio.ini).

You can install these directly through the Arduino IDE Library Manager or by cloning the repos.

## 🖥️ Supported Hardware

This project is designed for legacy Tacx **smart** trainers that do not natively support VS.

Tested so far:

- Tacx Neo (T2800)

Expected compatibility:

- Tacx Flux S (T2900S)
- Tacx Flux 2 (T2980)
- Most **smart** Tacx FE-C compatible trainers without VS-enabled firmware.

⚠️ If you test this on another model, please share your results in an issue or pull request so we can update the list.

## ⚡ Getting Started
1. **Install Arduino IDE 2.x** → [Download here](https://www.arduino.cc/en/software).  
2. **Clone this repo**:  
   ```bash
   git clone https://github.com/YOUR-USERNAME/Tacx-Virtual-Shifting.git
3. **Open Arduino IDE** → load the examples/ sketches.
4. **Install required libraries** (see below).
5. **Upload to your device** → run the example and try Virtual Shifting!

## ❤️ Contributing

This project is just starting!
If you’re interested in testing, coding, writing docs, or just giving feedback, contributions are welcome.

## ⚠️ Disclaimer

This is a community project and not affiliated with Tacx or Garmin.
Use at your own risk.
