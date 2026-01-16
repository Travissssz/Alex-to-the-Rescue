# Alex to the Rescue: Autonomous Search & Rescue Robot

An advanced remote-controlled search and rescue robot developed using an **Arduino Mega** for low-level hardware control and a **Raspberry Pi** for high-level processing. "Alex" is designed to navigate hazardous environments, utilising LiDAR mapping, high-precision odometry, and a robust teleoperation protocol.

---

## System Architecture

The robot utilises a master-slave architecture to separate time-critical hardware tasks from heavy computational processing:

* **Computational Hub (Raspberry Pi 4)**: 
    * Handles TLS-encrypted communication with the remote laptop.
    * Processes LiDAR data via **SLAM** (Simultaneous Localisation and Mapping) for real-time environment mapping.
    * Provides the High-Level UI for the operator.
* **Execution Hub (Arduino Mega)**:
    * Manages bare-metal ISR (Interrupt Service Routines) for wheel encoders.
    * Controls motor PWM signals and sensor data acquisition (Ultrasonic & Colour).
    * Communicates with the Pi via a custom UART packet protocol.



---

## Core Technical Features

### 1. High-Precision Odometry (Bare-Metal)
To ensure accurate movement in "dead-reckoning" scenarios, we implemented low-level interrupts:
* **External Interrupts**: Utilized pins 18 and 19 on the ATmega2560 to capture encoder ticks on falling edges.
* **Distance Calibration**: Calculated distance based on wheel circumference (20.42cm) and ticks per revolution, allowing for precise forward and backward movement routines.

### 2. Custom Communication Protocol
Data integrity between the Pi and Arduino is managed through a specialised packet system:
* **Packet Structure**: Includes a Magic Number for validation, Command/Response types, and a Checksum for error detection.
* **TLS Integration**: The transport layer is secured via OpenSSL, ensuring that control commands cannot be intercepted or tampered with.

### 3. Hazard Detection & Mapping
* **LiDAR Mapping**: Integrated a RPlidar A1 to generate 2D floor plans, allowing operators to see through "smoke" or around corners.
* **Colour Identification**: Uses a TCS3200 sensor to detect environmental waypoints (Red/Green/Blue/Orange) which trigger specific system responses.

---

## Technical Challenges Solved

- [x] **Power Management**: Resolved an issue where the Raspberry Pi would shut down due to voltage drops when motors were under high load. Implemented separate power rails and monitored battery discharge levels.
- [x] **Motor Calibration**: Fixed inconsistent 90-degree turns caused by battery discharge by implementing a software-based speed multiplier and timing delay adjustments.
- [x] **UART Serialization**: Developed a robust serialization/deserialization library in C++ to handle complex command packets without data loss.
- [x] **Signal Interference**: Shielded sensor wires to prevent electromagnetic interference from the DC motors from corrupting ultrasonic distance readings.

