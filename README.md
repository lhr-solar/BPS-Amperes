# BPS-Amperes

### Board Specification
- Measures battery current using low-side shunt resistor.

### Confluence
- https://cloud.wikis.utexas.edu/wiki/spaces/LHRSOLAR/pages/172360934/Amperes+Board

---

### Building / Testing
- To build main firmware, cd into Firmware folder and run ``` make ``` to build main files. 
- To build tests, cd into Fimrware folder and run ``` make TEST=test_name ```, where ```test_name``` is the name of the test without the .c suffix.
- Run ``` make help ``` for specific instructions.

### Flashing
- Attach USB device using USBIPD and flash via UART (``` make flash-uart ```) or ST-Link (``` make flash ```).
---
