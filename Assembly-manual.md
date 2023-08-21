# Simple Rotator Interface V. (release 2023)

Web interface with MQTT an USB support

-   [Web page](https://remoteqth.com/single-rotator-interface.php)
-   **[Order](https://remoteqth.com/order.php#rotator-interface)**
-   [Previous of version 4](https://remoteqth.com/w/doku.php?id=usb_rotator_interface_4)
-   [Previous of version 3](https://remoteqth.com/wiki/index.php?page=Rotator+module+version+3.3)

# Quick start guide

[Start in the Connection chapter](#POE-power-module)

# Hardware Supported:  
* **Three type azimuth potentiometer**
	- three wire from 500Ω to 10kΩ resistance
	- grounded two wire from 500Ω to 1kΩ
	- thre wire, used one turn from multiturn (typicaly 10kΩ)
* <del>Two azimuth **pulse** (CW/CCW) inputs</del> - NOT implemented yet
* **DC motor up to 28V/3A** with smooth start/finish (PWM)
* **AC motor max 50V/8A**
* **Brake** max 50V/8A
* **Natively supports [3D printed rotator](https://remoteqth.com/w/doku.php?id=3d-print-rotator)**
* Optional **external control unit [Gyrotator](https://github.com/ok1hra/gyrotator)**

## Source

-   **Firmware**
    -   **[GitHub source](https://github.com/ok1hra/IP-rotator)**
    -   **[Releases](https://github.com/ok1hra/IP-rotator/releases)**
-   **3D print model case**
    -   **[OpenScad
        source](https://remoteqth.com/download-count.php?Down=hw/SimpleRotatorInterfaceV.scad)**
    -   [Enclosure
        .stl](https://remoteqth.com/download-count.php?Down=hw/SimpleRotatorInterfaceV.stl)
    -   [Enclosure .stl for
        POE](https://remoteqth.com/download-count.php?Down=hw/SimpleRotatorInterfaceV-POE.stl)
        injector
    -   [Enclosure
        .stl](https://remoteqth.com/download-count.php?Down=hw/SimpleRotatorInterfaceV-M4mount.stl)
        (version mount on 3D print rotator)
-   **Electronics**
    -   [.PDF](https://remoteqth.com/download-count.php?Down=hw/SimpleRotaorInterfaceV-05.pdf)
        schematics
    -   [iBOM.html](https://remoteqth.com/download-count.php?Down=hw/SimpleRotaorInterfaceV-05-ibom.html)
-   Optional **3D print rotator**
    -   [Wiki](https://remoteqth.com/w/doku.php?id=3d-print-rotator)
-   Optional **external control unit**
    -   [Gyrotator](https://github.com/ok1hra/gyrotator)

# Functionality

-   **[Four mechanical mounting methods](#four-mounting-methods)**
-   **[Six control source at the same time](#six-control-source-at-the-same-time)**
-   **[Web interface](#web-interface)**
-   Remote **powered by POE** - need 13,8V to 15V/500mA source, mounted
    to 35mm DIN rail  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-17.jpg" width="300" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-17.jpg" />
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-18.jpg" width="150" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-18.jpg" />
-   Switching between **three and two wire azimuthpotentiometer** in
    setup page
-   Enable **preamp and shift op amp** in setup page
-   Local **control with CW paddle** via 3,5mm stereo jack connector
-   Galvanically isolated AC or DC motor control, including DC PWM
-   Working rotator **without hardware endstops** - replace by two
    (low/high) software adjustable forbidden zones in end positions
-   **MQTT** for integrated to your main ecosystem, typicaly NodeRed
-   **MQTT Wall** - web clent shown all topics included
-   **Overlap support,** for example 405° range, with rotate to short
    direction
-   Usable nearest to rotator, directly on tower, or in hamshack
-   **Status LED** while using front panel preset knob (indoor desktop
    use only)
-   **LED showing fuse failure** on main board or POE interface  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-68.jpg" width="300" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-68.jpg" />
-   Three safe stop **watchdog** - 60 sec after firmware without answer,
    max 90 sec rotate, max 10 sec without 10 grad azimut change during
    rotate
-   Under 11V voltage POE power watchdog
-   Warm up timer for stable value during setup
-   Firmware detects the hardware version and displays it in the web
    interface
-   Optionaly controled (via MQTT) by Gyrotator independent hardware
    based on M5Stack fire [Gyrotator](https://github.com/ok1hra/gyrotator)

## Four mounting methods

<table>
<tbody>
<tr class="odd">
<td>Outdoor <strong>on tower</strong><br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-01.jpg" width="350" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-01.jpg" /><br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-02.jpg" width="350" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-02.jpg" /></td>
<td>Outdoor, <strong>part of the <a href="https://remoteqth.com/w/doku.php?id=3d-print-rotator">3D printed rotator</a></strong> (optional)<br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-05.jpg" width="400" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-05.jpg" /></td>
</tr>
<tr class="even">
<td>Indoor on <strong>35mm DIN rail</strong><br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-03.jpg" width="300" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-03.jpg" /></td>
<td>Indoor <strong>to your desktop</strong> (optional)<br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-04.jpg" width="400" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-04.jpg" /></td>
</tr>
</tbody>
</table>

## Six control source at the same time

<table>
<tbody>
<tr class="odd">
<td><strong>Web interface</strong><br />
with azimutal world map,<br />
containing the current grayline<br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-06.png" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-06.png" /></td>
<td><strong><a href="https://github.com/ok1hra/gyrotator">Gyrotator</a></strong><br />
Optional controller on external hardware <a href="https://shop.m5stack.com/products/fire-iot-development-kit?variant=16804798169178">5Stack FIRE</a><br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-07.jpg" width="350" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-07.jpg" /><br />
<img src="https://youtu.be/e5L1Iu4h3rg" alt="YouTube video" /></td>
<td><strong>MQTT</strong><br />
(<a href="https://github.com/bastlirna/mqtt-wall">mqtt-wall</a> include)<br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-08.png" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-08.png" /></td>
</tr>
<tr class="even">
<td><strong>USB GS-232 protocol</strong><br />
(commands: R L A S C Mxxx O F)<br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-09.png" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-09.png" /></td>
<td><strong>CW paddle</strong><br />
- indoor desktop use<br />
- or for manipulate with rotator on DIN rail, or on mount on tower<br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-10.jpg" width="350" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-10.jpg" /></td>
<td>Optional front panel <strong>preset knob</strong><br />
works independently without a GUI, together with the status LED (indoor desktop use only)<br />
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-11.jpg" width="150" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-11.jpg" /></td>
</tr>
</tbody>
</table>

## WiFi e-ink display
- **[GitHub page](https://github.com/ok1hra/esp32-e-ink)** of the project

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-72.jpg" width="410">

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-71.jpg" width="200">  <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-73.jpg" width="200">

- **The last azimuth remains on the display even after turning off the rotator.**
- Optional display on external hardware from [LaskaKit - ESPink-42 ESP32 e-Paper](https://www.laskakit.cz/laskakit-espink-42-esp32-e-paper-pcb-antenna/)
- It is not used to control the rotator, only to display its rotated state.


## Web interface

- Simply [setup page](#setup-web-page) and calibrate in three steps

  <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-12.png" width="200"> <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-13.png" width="150">
- Adjustable width of the antenna radiation angle

  <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-14.png" width="150">
- Rotate with one click to map
- If click during run, stopped rotation
- Smaller range than 360° shows end limits

  <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-15.png" width="150">
- Show connected status between web page and rotator

  <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-16.png" width="150">

------------------------------------------------------------------------

# How to assembly

## Need for assembly and run

1.  **Solder station**
2.  Two **Allen key** 2 and 2.5 mm
3.  Flat **screwdriver,** 2.5 mm wide
4.  Ohmeter (for ten turn azimuth potentiometer only)
5.  Drill with a diameter of 6 mm (optional for front panel)
6.  Two **RJ45 ethernet cable** for connecting to local network with
    your prefered length.
7.  Local ethernet network with running DHCP server.
8.  13.8V/0,5A **power supply** for POE injector
9.  Power suply for motor yours rotator. AC or DC and voltage according
    to type.

## Assembly steps

1.  Check that all components are complete  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-19.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-19.jpg" />
2.  3D printed components + two M3 x 10mm screws  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-20.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-20.jpg" />
3.  PCB with SMT components  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-21.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-21.jpg" />
    1.  If the PCB is delivered in a 3D printed box, you can remove it
        by lifting the PCB at the place where the plastic stop is and
        pulling it towards you - see picture  
        <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-40.png" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-40.png" />
        <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-39.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-39.jpg" />
4.  Solder the DC jack first  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-22.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-22.jpg" />
5.  For better mounting, fit the fuse terminals with fuses  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-23.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-23.jpg" />
6.  Solder the 3A version between the input connectors  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-24.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-24.jpg" />
7.  Solder the 0.5A version to the POE module  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-25.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-25.jpg" />
8.  Check the fuse values carefully. The opposite assignment will cause
    a failure  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-26.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-26.jpg" />
    1.  0.5A in the POE module
    2.  3A between input connectors
9.  In the next step, solder two RJ45 connectors to the POE module  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-27.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-27.jpg" />
10. Solder the diode D24 - **attention to the polarity, the white strip
    is on the side marked K**  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-28.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-28.jpg" />
11. Use a screwdriver to unlock the metal casing of the RJ45 connector  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-29.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-29.jpg" />
12. Remove the metal cover and discard it  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-30.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-30.jpg" />
13. Solder the connector to the board  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-31.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-31.jpg" />
14. The PCB board currently looks like this  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-32.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-32.jpg" />
15. Solder the two (blue) potentiometers so that the control screws are
    facing outwards from the board  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-33.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-33.jpg" />
16. Now solder the three relays  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-35.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-35.jpg" />
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-34.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-34.jpg" />
17. Carefully break off the POE module  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-36.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-36.jpg" />
18. Use pliers to break off the remaining bridges  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-37.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-37.jpg" />
19. If you **don't install [optional front
    panel](#assembly-optional-front-panel)** solder the SMT jumper J19  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-38.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-38.jpg" />
20. Now you can slide the board into the slots in the 3d printed box
    so  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-39.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-39.jpg" />
21. that the center latch engages when fully inserted  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-40.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-40.jpg" />
22. The back cover is held with its two latches  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-41.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-41.jpg" />
23. We insert the POE module into the 3D printed box  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-42.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-42.jpg" />
24. And screw the lid on with two screws. **Attention, we tighten
    lightly,** the screws are screwed directly into the plastic  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-43.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-43.jpg" />
    1.  The side to which the power supply **DC jack** is connected is
        intended for connecting **electronics**
    2.  The supply voltage is **13.8-15V DC (500mA)** with any polarity
    3.  **Breakage of the fuse** in the POE module is **signaled by the
        LED** below the fuse  
        <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-68.jpg" width="300" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-68.jpg" />

------------------------------------------------------------------------

## Assembly optional front panel

Front panel and potentiometer control is an optional extension. All
other control options, including the web interface, remain and work in
parallel. Only the other options will not turn the front panel
potentiometer, so if used, the front panel potentiometer setting will
not match.

1.  Check the presence of all necessary components  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-44.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-44.jpg" />
2.  Solder the connection cable to the potentiometer. **Attention, it is
    necessary to observe the positions of the colors of the wires**  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-45.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-45.jpg" />
3.  Glue the four rubber feet to the underside of the 3D printed model
    as shown  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-46.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-46.jpg" />
4.  Force push/punch the plug in the center of the front of the 3D
    printed box  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-47.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-47.jpg" />
5.  Drill the indicated hole in the front part of the box, intended for
    the LED, with a 6mm drill bit  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-48.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-48.jpg" />
6.  Insert the potentiometer with the soldered jumper from the inside
    into the prepared hole and tighten the nut slightly  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-49.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-49.jpg" />
7.  Place the front panel and screw in two M3 x 10mm screws - tighten
    the last threads carefully, the screw is screwed directly into the
    plastic  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-50.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-50.jpg" />
8.  When using the potentiometer on the front panel, **jumper J19 must
    be open.** If it is shorted, remove the tin connector by suction
    with a wire puller  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-52.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-52.jpg" />
9.  Put the knob on and gently tighten with a flat screwdriver -
    calibration of the exact setting is done later from the web
    interface, setup page. More in the [Setup and calibrate
    section](#setup-and-calibrate)  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-51.jpg" width="350" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-51.jpg" />
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-13.png" width="100" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-13.png" />
10. Solder LED diode wit orientation **beveled side into the slot marked
    R**  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-59.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-59.jpg" />
11. Overhang of the diode over the edge of the PCB at least 12.5 mm  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-53.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-53.jpg" />
12. The distance above the PCB should be 4.0 mm  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-54.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-54.jpg" />
13. Insert the PCB into the guide grooves of the box about halfway and
    plug in the potentiometer connector. Pay **attention to the
    polarity,** the connector can only be inserted in one position.  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-55.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-55.jpg" />
14. Insert the whole PCB into the box until the locking tooth in the
    middle engages.  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-56.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-56.jpg" />
15. After installing the back cover, the upgrade to the front panel is
    complete.  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-57.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-57.jpg" />
16. You can continue by connecting the rotator.  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-58.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-58.jpg" />

------------------------------------------------------------------------

# Upload firmware

## Compile

1.  **Install [Arduino IDE](https://www.arduino.cc/en/software)** rev
    1.8.19
1.  **Install support [for
    ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)**
1.  **Install** these **libraries** in the versions listed
	* WiFi rev 2.0.0
	* EEPROM rev 2.0.0
	* WebServer rev 2.0.0
	* Ethernet rev 2.0.0
	* ESPmDNS rev 2.0.0
	* ArduinoOTA rev 2.0.0
	* Update rev 2.0.0
	* AsyncTCP rev 1.1.1
	* ESPAsyncWebServer rev 1.2.3
	* FS rev 2.0.0
	* AsyncElegantOTA rev 2.2.7
	* PubSubClient rev 2.8
	* Wire rev 2.0.0
1. **Select board** 'OLIMEX ESP32-PoE'
1. **Connect** the rotator with a **USB-C** cable and select the corresponding port in the arduino IDE
1. Now you can **compile and upload** code using USB

## Compile + OTA update

The firmware can be **uploaded via Ethernet** under the following
conditions

1.  The Simple rotator board **already contains firmware**
2.  The Simple rotator board is **connected to the Ethernet with a
    DHCP** server
3.  The Simple rotator board is **powered by a POE** module

Now in the **Arduino IDE** you can select the /Tools/Port/Network port
in the menu, the item starting with ROT with the IP address
corresponding to your rotator.  
At this point it is **possible to compile and upload** the firmware
using the Arduino IDE.  
The Arduino IDE will ask for a **password** during the upload, which is
'remoteqth'.  

## Upload binary via web interface

If the [previous three conditions](#Compile-+-OTA-update) are met, you can upload
the firmware binary using the **web interface**.

1.  **Open url 'http://[YOUR IP]:82/update'**
    -   Also available from the header of the first page as a link **| Upload FW |** <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-60.png" width="350" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-60.png" />
2.  **Download** last release **.bin** file from [GitHub](https://github.com/ok1hra/IP-rotator/releases)
3.  **Upload .bin file** via web form, with the **Firmware** option selected  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-61.png" width="350" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-61.png" />
4.  **ATTENTION,** after uploading, it is necessary to **restart** the rotator by disconnecting and reconnecting the power supply.

------------------------------------------------------------------------

# Connection

**General description** of connectors, individual variants detailed below  
  
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-connect.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-connect.png" />

------------------------------------------------------------------------

## POE power module

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-power.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-power.png" />  

-  **LED showing if fuse failure** on main board or POE interface  
   <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-68.jpg" width="300" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-68.jpg" />

------------------------------------------------------------------------

## Azimuth potentiometer

Note the connection polarity for systems that share a GND potential  
  
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-az-pot.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-az-pot.png" />

1.  The **type potentiometer** used must also be set in the web
    interface, [Setup page.](#setup-web-page) **Option "Azimuth
    potentiometer" 2wire/3-wire**  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-12.png" width="100" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-12.png" />
2.  When using one turn of a **ten-turn potentiometer,** also **activate
    the "Azimuth gain/shift op-amp" option to ON.** About calibrate more
    in [Calibrate section](#calibrate-web-page).
3.  If you are using a **two-wire 500Ω potentiometer,** short-circuit
    jumper J16 on the bottom side of the PCB. **Do not use this jumper
    in other cases.**  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-69.jpg" width="250" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-69.jpg" />

------------------------------------------------------------------------

## AC motor

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-ac.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-ac.png" />

------------------------------------------------------------------------

## DC motor with PWM

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-dc-pwm.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-dc-pwm.png" />  

-  **Activate** the PWM functionality on the [Setup webpage](#setup-web-page)
-  **ATTENTION**
	-  Observe the **polarity of the power supply,** otherwise the power mosfet may be destroyed.  
	-  Observe the **maximum prescribed loading.**
	-  We recommend using the **following safe option with software endstops.**

------------------------------------------------------------------------

## DC motor with PWM, without hardware endstop (safe mode)

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-dc-pwm-safe.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-dc-pwm-safe.png" />  

1.  If you use a **rotator without hardware endstops,** the destruction of one component (power mosfet) can cause the rotator to crash. Therefore, it is safe to connect one more active element in series (relay intended for the brake).
1.  **Activate** the PWM functionality on the [Setup web page](#setup-web-page)
2.  **ATTENTION**
    -  observe the **polarity of the power supply,** otherwise the power mosfet may be destroyed.
    -  observe the **maximum prescribed loading.**

------------------------------------------------------------------------

## DC motor without PWM

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-dc.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-dc.png" />  
- **PWM can be replaced** with an external brake relay

1.  **Deactivate** the PWM functionality on the [Setup web
    page](#setup-web-page)
2.  **ATTENTION**
    1.  Don't forget to **bridge or replace the diode D24 with a
        jumper.**
    2.  Make the **jumper with a copper wire** with a cross-section of
        1.5 mm<sup>2</sup>  
        <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-67.jpg" width="400" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-67.jpg" />

------------------------------------------------------------------------

# Control, setup and calibrate

## LED status

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-led.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-led.png" />  

-  **LEDs signal** static (PWR) or dynamic (PWM) states during rotator operation

------------------------------------------------------------------------

## Connect and tune

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-setup.png" width="800" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-setup.png" />

1.  The **fuse** for the motor should be optimally 5A
2.  **Two blue, multi-turn potentiometers** are used to set the Shift
    and Gain of the preamplifier in the azimuth measurement mode with
    the use of one turn from the ten-turn potentiometer.
3.  The **USB-c connector** is used for uploading firmware or connecting
    a serial console. **DO NOT use USB-C to power electronics.**
4.  Internal three-pin connector (balls), serves for connecting an
    **[optional front panel
    potentiometer](#assembly-optional-front-panel).**
5.  The 3.5mm stereo jack serves for connecting a manual control of the rotator, typically a **CW bug.**  
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-jack.png">
    <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-10.jpg" width="500" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-10.jpg" />

------------------------------------------------------------------------

## How to find the IP address of the device

1.  [First is need](#poe-power-module)
    1.  connect POE power supply
    2.  ethernet to a site with an active DHCP server
2.  Variant via **USB terminal**
    1.  connect USB-C between rotator and PC
    2.  open Arduino terminal with 115200 baud speed
    3.  press ? and enter
    4.  terminal show your IP address  
        <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-09.png" width="300" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-09.png" />
3.  The variant can be **found using the arduino IDE**
    1.  Open Arduino IDE menu **Tools/Port/Network port**
    2.  The menu will display network devices that contain OTA
        firmware - the IP address is at the end of the line starting
        with **ROT-**  
        <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-63.png">
4.  With the help of the **Fing android application**
    1.  **Install**
        [Fing](https://play.google.com/store/apps/details?id=com.overlook.android.fing)
    2.  **Scan** local network
    3.  **Find** device with brand name Espresiff and begin MAC start
        08:B6:1F  
        <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-64.png" width="150" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-64.png" />

------------------------------------------------------------------------

## Settings using the web interface

### Main web page

**Open the IP address** found using the previous instruction **in a web
browser** http://YOUR-IP  
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-08.png" width="400" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-08.png" />  
How to works
- The first page contains an **information header,** including a link to the **Firmware Upload** page and the **Release page** on GitHub. Give **green the button of the link to the control page** of the rotator. Below it is the **web MQTT client** [credit MQTT wall](https://github.com/bastlirna/mqtt-wall) displaying the data sent or received by the rotator from the MQTT broker - data used to **control or debug** the rotator.

------------------------------------------------------------------------

### Control web page

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-06.png" width="400" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-06.png" />  
How to works

- The basis for the display is an **azimuthal map,** centered in your QTH. The map shows the current grayline. Map settings are made in the Setup page, item Background azimuth map URL.
- The **direction of rotation indicator** is a transparent triangle whose angle corresponds to the width of the antenna's radiation pattern. The center is marked with a green pointer on the scale. The width of the radiation diagram is set in the Setup page, item Antenna radiation angle in degrees.
- **COLORS -** If the direction is in the free range, the indicator color is green. If the pointer reaches the overlap zone, the color is yellow. If the rotator is in rotated mode, the indicator color is red. If the rotator reaches the endpoints, the color is gray.
- The **yellow line on the edge** shows the Overlap zone. It is the angle by which one 360-degree revolution is extended.
- **Gray bar on the bottom** shows. | Antenna name - set in the Setup page, Rotator name item. | The power supply voltage controls the electronics from the POE injector - if it drops below 11V, the watchdog stops the rotator. | Raw angle rotated - angle without displacement, as if it were zero degrees north. | Link to the Setup page.
- **Below the gray bar** is shown, **MAC** address of the Ethernet interface and website **connection status** with engine electronics. If it is not connected, the data on the website is not up to date and the rotator cannot be controlled.
- **CONTROL -** by tapping on the map, the azimuth is calculated and the rotation starts. **Emergency stop is performed by clicking anywhere on the azimuthal map again.**
   - **Tip: for a short movement of the rotator, you can click on the map twice in close succession. The motor will only turn on for this time, including the PWM start/stop sequence.//**

------------------------------------------------------------------------

### Setup web page

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-12.png" width="400" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-12.png" />  
How to works

- The Setup page is used to set the basic properties of the rotator.
- You will **notice that the hints** for individual items are hidden under the question mark at the end of the line.
- **Your callsign and Rotator ID** are unique identifiers used to generate an MQTT topic in the form [callsign]/[ID]/ROT/[variable]. Changing these values requires a reload of the first page with the web MQTT client. //After the first power-on, for the Callsign firmware uses the last five characters from the MAC address until a new setting is made.//
- **Rotator name** is used to identify the rotator on the web control page.
- **Start CCW azimuth** is the end value of the start of rotation in the counter clockwise direction. This is the azimuth that the rotator is rotated in real conditions, if it is in this end position.
- **Rotation range in degrees** is the rotation range of the rotator between two (CCW and CW) endpoints in degrees.
- **Background azimuth map URL** is a link to a web file with a bitmap azimuth map. Generation of own map is described in chapter [Run own services](#background-azimuth-map-generator-with-grayline)
- **Antenna radiation angle in degrees,** is the value of the width of the radiation angle of your antenna, which will be applied in the display of the control page.
- **Azimuth source** is a choice between a signal source for azimuth determination. The potentiometer is the default value. The CW/CCW pulse function is then not implemented now.
- **The azimuth potentiometer** is a choice between two and three-wire variants. **The two-wire** variant uses a constant current source of 9mA and enables the connection of a potentiometer **smaller than 1kΩ.** The **three-wire** variant uses a constant voltage source of 9V and enables the connection of a potentiometer of **500Ω or more.** You can find the connection in the [section.](#azimuth-potentiometer|Azimuth potentiometer)
- **Azimuth gain/shift op-amp,** is turned ON, **ONLY when using a multi-turn three-wire potentiometer,** of which only one turn is used. Because one turn generates a small change at the input of the AD converter, it is possible to help amplify the preamplifier. How to do it:
  - The first step is to **set the trimmers R80 and R81 to the default position.** Connect the ohmmeter to the measuring point S (J1, J12) and turn the trimmer R81 counterclockwise until the ohmmeter shows a value close to 0Ω. We repeat the same thing with trimmer R80 and measuring points and G (J13, J14).
  <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-65.jpg" width="200"> <img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-66.jpg" width="200">
  - This setting **assumes that REVERSE-CONTROL and REVERSE-AZIMUTH are correctly set** in the Calibration page.
  - The **second** step is to physically turn the rotator up to the CCW endstop, which will not cause the AD converter input to shift to the left edge of the scale on the Calibration page. Therefore, we start moving the value towards the left endstop on the scale by turning the potentiometer S (R81) clockwise until we reach the left endstop on the scale, ATTENTION, after reaching the edge of the scale, do not continue turning the potentiometer, the input value of the AD converter must still move within the range scale.
  - The **third** step is to physically turn the rotator up to the CW endstop, which will not cause the AD converter input to shift to the right edge of the scale on the Calibration page. Therefore, we increase the gain of the preamplifier by turning the potentiometer G (R80) clockwise until we reach the right endstop on the scale. ATTENTION, after reaching the edge of the scale, do not continue turning the potentiometer, the input value of the AD converter must still move within the range of the scale.
  - **We repeat** the previous two steps until we achieve the desired result.
  - Then **calibrate** the SAVE-CCW and SAVE-CW endpoints on the Calibration Page.
- **Endstops INSTALLED,** turning off this item activates software endstops, which are prohibited zones on the edges. The size of these zones is set in the following two items. If this item is enabled, the firmware relies on the hardware endstops in the rotator, including that they are not beyond the range of the measure potentiometer.
- **CW/CCW forbidden zone (software endstops),** are values of the range of forbidden zones in millivolts. the entire range of the rotator represents a voltage of 0 to 3.3V. The setting value will appear in the Calibration page as a yellow area at the edges of the range. If the rotator is set correctly, it only allows movement in the direction from the edge to the center. Keep the protection zone large enough to prevent damage to the rotator.
- **Watchdog speed** is the minimum rotation speed in seconds per one turn, which if the rotator does not reach, it will be stopped by the watchdog. Use a value at least 50% higher than the real speed to avoid false stops.
- **Motor supply,** is a choice between AC and DC rotator type. The DC type enables the activation of the PWM start-up and run-down ramps.
-  **DC PWM control** activates the PWM ramp-up and ramp-down when using the DC rotator. If you use a rotator without hardware endstops, the destruction of one component (power mosfet) can cause the rotator to crash. Therefore, it is safe to connect one more active element in series (relay intended for the brake). **This setup is highly recomended, see [Connection section](#dc-motor-with-pwm-without-hardware-endstop-safe-mode).**
- **USB serial BAUDRATE,** is the setting of the communication speed of the serial console on the USB-C connector using the GS-232 protocol. Enabled commands
  * **?** display the IP address of the rotator
  * **R** clockwise rotation
  * **L** counter clockwise rotation
  * **A or S** CW/CCW rotation Stop
  * **C** show antenna direction value
  * **Mxxx** rotate to azimuth, degree represent with three number
  * **O** CCW calibration
  * **F** CW calibration
- **MQTT broker IP,** IPV4 address of the MQTT broker server
  - If the first digit is zero, MQTT is **disabled**
- **MQTT broker PORT,** number of the IP port on which the MQTT broker runs.
- **Change** button, save actual value.
- **Calibrate** button open Calibration web page.

------------------------------------------------------------------------

### Calibrate web page

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-13.png" width="400" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-13.png" />  
Calibration is divided into three steps

- **Rotate direction calibrate** So that it is not necessary to observe the polarity of the rotator motor connection, you can use the CCW, CW and STOP buttons to test the direction of rotation. **If the rotator rotates in a different direction** than the control button, you can reverse the direction using the REVERSE-CONTROL button.
- **Azimuth calibrate** The **scale shows** the status of the input AD converter in the range of 0 to 3.3V. Due to activated software linearization, regions in the range of tens of mV are not available. If **software endstops are activated** in the Setup web page, the prohibited zones at the edges of the range will be displayed in yellow. In the lower part there is an arrow showing the current read value of the AD converter. There are gray **arrows on the edges, showing the calibration values** of CCW and CW stops. How to calibrate:
  - Turn the rotator briefly using the CW, CCW and STOP buttons in the previous section and watch the movement of the current value of the AD converter. The correct direction is left for CCW and right for CW. **If the arrow moves in reverse,** invert it using the REVERSE-AZIMUTH button
  - Then **rotate the rotator counterclockwise** until it is stopped by a software or hardware endstop. After stopping, save the value with the help of the SAVE CCW button.
  - Continue **rotating the rotator clockwise** until it is stopped by a software or hardware endstop. After stopping, save the value with the help of the SAVE CW button.
  - The **RESET CW/CCW SAVE button** is used to set the default values if the setting needs to be repeated.
  - **After calibrate,** rotate to full CCW limits, measure real azimuth, and put this value to “Start CCW azimuth:” field in Setup page.
  - Changes to settings in the map control page will be **reflected after the page is reloaded.**
- **Front panel calibrate (optional)** if install [Optional front panel](#assembly-optional-front-panel) you can calibrate knob
  - Rotate front panel potentiometer axis without knob to value 0°
  - Put knob with orientation to north on axis
  - Fixate knob to axis on position north

------------------------------------------------------------------------

## Control via optional front panel

<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-04.jpg" width="400" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-04.jpg" />  
The front panel contains only two active elements:

1.  **Button for setting the required azimuth** - if the front panel is
    installed, turning the knob will make a **quick change of the target
    azimuth,** but it is not very accurate. The rotation of the rotator
    can be monitored on the control page of the web interface, or via
    MQTT, optionally also on the Gyrotator unit. **Attention,** the
    direction of the control knob on the front panel will no longer
    correspond to the real rotation of the rotator, if you change its
    azimuth from another source, for example from a web browser.
2.  **The status LED has three states**
    1.  **RED** lights up during rotating.
    2.  **GREEN** lights up if the rotator has reached the set azimuth
        and stopped.
    3.  **FLASHES GREEN** if the rotator has stopped and has not reached
        the target azimuth.

------------------------------------------------------------------------

## Manually controll

Manual control is available via the **3.5mm stereo jack** connector on
the back of the electronics. By switching the left or right channel, it
is possible to control clockwise or counterclockwise rotation.
**[Connection is here.](#connect-and-tune)**  
Your **CW key** can easily be used as a switch control.  
This control is suitable for service movements or if you operate the
Interface on your desk.  
<img src="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-10.jpg" width="350" alt="https://raw.githubusercontent.com/ok1hra/IP-rotator/main/img/wiki-simple-rot-10.jpg" />

------------------------------------------------------------------------

# Running own services

## MQTT broker

MQTT is a universal protocol that is used to communicate the rotator
with other devices. The control node transmitting this information is
the MQTT broker. The default settings of the rotor use the **public MQTT
broker of the Internet at the address remoteqth.com.** In order to avoid
dependence on an Internet connection or the availability of a public
MQTT broker, **we recommend that you install your own MQTT broker** on a
Raspberry PI or other device in the local network. For this you can
**use some of the public instructions like
[this](https://www.makeuseof.com/install-mqtt-server-node-red-raspberry-pi-home-automation/)
one** (without installing NodeRed).  
After installation, **change the settings** of two items (MQTT broker IP
and MQTT broker port) in the Setup section, according to the IP address
of your local MQTT broker.

## Background azimuth Map generator with grayline

The RemoteQTH server provides a live grayline world map generation for
selected DXCC countries at address <https://remoteqth.com/xplanet/>
(adding others upon request from OK1HRA). The entire path to the map is
entered in the Setup page in the item Background azimuth map URL.  
If you don't want to depend on the internet connection or remoteqth.com
server functionality, you can run the real generator on any Linux
machine, for example Raspberry PI. You need two functions for this:

1.  http server, **[example here](https://thepi.io/how-to-set-up-a-web-server-on-the-raspberry-pi/)**
2.  Script generating bitmap
    1.  Install xplanet with code
        `sudo apt install xplanet xplanet-images`
    2.  Create line in /etc/crontab
        `*/15 *  * * *   web     /usr/bin/xplanet -window -longitude 13.8 -latitude 50.0 -geometry 600x600 -projection azimuthal -num_times 1 -output /var/www/remoteqth/xplanet/OK.png`
        Where to replace **web user** a valid user of your system |
        numerical coordinates of **longitude and latitude** with your
        requested coordinates | **path** /var/www/remoteqth/xplanet/
        for the path published by your web server | **file name**
        OK.png for your desired file name
    3.  Enter the resulting path to the generated bitmap in the
        Background azimuth map URL item on the [Setup page](#setup-web-page).

------------------------------------------------------------------------

# Known bugs

-   If the continuous draw of your motor does not exceed the allowed
    limit, but the fuse still blows, the problem may be in the starting
    current. This current is naturally limited by the length of the
    power cable. If the problem persists even with the rotator on the
    mast, supplement the connection with a starting resistor
    ([B57237S0109M](https://www.tme.eu/cz/details/b57237s0109m/ochranne-termistory-ntc/epcos/))
    in series with the rotator motor.
