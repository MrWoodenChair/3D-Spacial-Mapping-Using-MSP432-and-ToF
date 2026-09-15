# MSP432 ToF 3D Scanner

A real-time 3D space-scanning system built for COMPENG 2DX3 (McMaster University). A Time-of-Flight distance sensor is rotated by a stepper motor to sweep a plane; the microcontroller streams distance/angle data over UART to a MATLAB script, which converts polar readings to Cartesian coordinates and renders a live scatter plot and wireframe mesh of the scanned space.

## Hardware

| Component | Details |
|---|---|
| Microcontroller | Texas Instruments MSP432E401Y (Cortex-M4F, 26 MHz, 1024 KB flash, 256 KB SRAM) |
| Distance sensor | VL53L1X Time-of-Flight sensor (I2C, up to 360 cm range) |
| Actuation | 28BYJ-48 stepper motor + ULN2003 driver board (512 steps/revolution) |
| Host link | UART @ 115200 bps to PC (COM port) |

See `docs/Final_Project_Report_Vaishnav_Jayaraj.pdf` for the full circuit schematic, characteristics table, and design writeup.

## How it works

1. Pressing the onboard button (PJ0) triggers an interrupt that sets a scan-start flag.
2. The firmware rotates the stepper motor through a full 512-step revolution, pausing at fixed intervals to take a ToF distance reading over I2C.
3. Each reading (`virtual_z, angle, distance`) is sent over UART as a CSV line.
4. After each full rotation, the motor unwinds (to prevent wire twisting) and the "layer" (`virtual_z`) is incremented — this is where you physically shift the sensor rig along the axis being scanned.
5. On the PC side, the MATLAB script listens for `START`, reads incoming lines, converts each `(angle, distance)` pair to Cartesian `(y, z)` via trigonometry, and plots points live. On `END`, it renders a final wireframe by connecting points within each layer and between adjacent layers.

## Repo layout

    firmware/   Keil project + C source for the MSP432E401Y
    matlab/     Data collection & visualization script, plus demo scan figures
    docs/       Final report and VL53L1X sensor docs
    media/      Images and videos related to the project

## Building the firmware

1. Open `firmware/2dx_studio_8.uvprojx` in Keil µVision.
2. Adjust `MEASUREMENTS_PER_TURN` and `NUM_OF_SCAN_LAYERS` in `2dx_studio_8.c` if you want a different scan resolution or layer count (defaults: 128 measurements/rotation, 3 layers).
3. Connect the board, select the correct debugger target, then **Translate → Build → Load**.

## Running the MATLAB visualization

1. Open `matlab/DX3_Final_Project_code.m`.
2. Set `port` to your board's COM port (check Device Manager → Ports → "XDS110 Class Application/User UART").
3. If you changed `MEASUREMENTS_PER_TURN` / `NUM_OF_SCAN_LAYERS` in firmware, update `points_per_rotation` / `num_layers` in the script to match.
4. Run the script, then reset the microcontroller. Wait for LED1 to flash twice (ready), then press the start button (PJ0).
5. After each rotation completes, physically shift the rig along the scan axis by a fixed displacement before the next layer begins.

## Limitations

- Single-precision FPU on the MCU is not used for `sin`/`cos`; trig is computed in MATLAB (double precision) instead, trading onboard-processing efficiency for accuracy.
- Stepper speed is capped to avoid stalling and overheating; motor stop/start between scan and rotation stages limits overall scan speed.
- UART throughput is bounded by the PC's serial port limits (115200 bps used here), independent of the MCU's higher theoretical UART ceiling.

## Acknowledgements

Parts of this code build on material provided in COMPENG 2DX3 - Microprocessor System Project, taught by Dr. Thomas Doyle, Dr. Shahrukh Athar, Dr. Yaser Haddara, Dr. Mohamed Elamien, and Dr. Omar Boursalie. Some of the sensor driver code also comes from Texas Instruments and STMicroelectronics.

This repo is shared for reference and portfolio purposes. If you're currently taking 2DX3, copying this code for your own submission is a violation of McMaster's Academic Integrity Policy — please do your own work.
