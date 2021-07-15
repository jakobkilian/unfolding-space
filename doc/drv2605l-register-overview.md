# Overview of register programming on the DRV2605l

The DRV has 8-bit address and data bytes which are transferred with the most-significant bit (MSB) first. To write and read we currently still use the *deprecated* WiringPi library functions (e.g. wiringPiI2CWriteReg8).



## Address and Multiplexing

The DRV boards only have one unmutable i2c address: 

- **0x5A** (7-bit) or **1011010**
- **0xB4** (8-bit) or **10110100** for writing

- **0xB5** (8-bit) or **10110101** for reading

Therefore I use a i2c multiplexer (PCA9635 or TCA9584a) to route i2c signals to the targeted DRV. There may be other ways to solve this (enable/disable chips, using pwn, ...) but due to time restriction we stuck with this solution for the moment.



## Registers

R = read only, X = Read & Write

Defaults are the bold bits on the left side



### 0x00 – STATUS

**111**0 0000 | R | DEVIDE_ID  – Always the same as we use DRV2605L 

000**0** 0000 | – |

0000 **0**000 | R | **DIAG_RESULT** ➔ **0: OK**, 1: failed, can be calibration or diagnostic mode

0000 0**0**00 | R | **FB_STS** ➔ **0: OK**, 1: timed out; (EMF Feedback Status)

0000 00**0**0 | R | **OVER_TEMP** ➔ **0: OK**, 1: too hot

0000 000**0** | R | **OC_DETECT** ➔ **0: OK,** 1: over current detected



### 0x01 – MODE

**0**000 0000 | X | DEV_RESET ➔ if set (to 1) device gets reset like at power up

0**1**00 0000 | X | STANDBY ➔ if set (to 1) device enters standby

0000 0**000** | X | **MODE **➔ **5: controlled by real-time-playback (i2c),**  6: diagnosis, 7: calibration

---

Start with Mode 7, when calibrated got to Mode 5 and write values on 0x02



### 0x02 – RTP / Real-Time-Playback 

**0000 0000** | X | **RTP_INPUT** ➔ 0 - 255 Vibration strength

---

When 0x01 is in Mode 5, we can write our vibration intensity value (0-255) here



### 0x0C – GO

0000 000**0** | X | **GO ➔ fires events like calibration and diagnosis**

---

Fires events e.g. from 0x01 Mode. Also triggers waveforms and other stuff



### 0x16 – RATED VOLTAGE

**0011 1111** | X | RATED_VOLTAGE ➔ calculated value; 0-255; depending on motor's rms voltage

---

Set before calibrating! Gets calculated by a Formula in **7.5.2.1** / p.22 of the datasheet by using rms and freuquency of the motors datasheeet. Note: Sample Time is microseconds. So use Sample Time * 10^{-6}.

Use https://www.symbolab.com/: solve\:for\:x,\:\:RMSVOLTAGE=\frac{20.71\cdot 10^{-3}\cdot \:x}{\sqrt{1-\left(4\cdot 300\cdot 10^{-6}+300\cdot 10^{-6}\right)\cdot FREQUENCY}}

*G1040003D:* 104 (Frequency: 175Hz)

*G0832012:* 70 (Frequency: 235Hz)



### 0x17 – OVERDRIVE CLAMP VOLTAGE

**1000 1001** | X | **OD_CLAMP** ➔ calculated value; 0-255; depending on motor's max voltage

---

Gets calculated by a Formula in **7.5.2.2** / p.23 of the datasheet by using the max allowed voltage of the motors datasheeet. If there is none guess one

Use https://www.symbolab.com/: solve\:for\:x,\:MAXVOLTAGE\:=\:21.96\cdot 10^{-3}\cdot \:x

*G1040003D:* 150 (guessed 3,3V)

*G0832012:* 91 ((guessed 2V)



### 0x18 – AUTO-CALIBRATION COMPENSATION

**0000 1101** | X |  A_CAL_COMP ➔ compensation value for resistive loss in the driver

---

Gets set in calibration routine. Can be saved on the Raspi and written on the chip instead of calibration?



### 0x19 – AUTO-CALIBRATION BACK EMF

**0110 1101** | X |  A_CAL_BEMF ➔ compensation value for resistive loss in the driver

---

Gets set in calibration routine. Can be saved on the Raspi and written on the chip instead of calibration?

Back EMF is the voltage that the actuator gives back when the actuator is driven at the rated voltage.



### 0x1A – FEEDBACK CONTROL

**1**000 0000 | X | **N_ERM_LRA** ➔ 0:ERM; **1:LRA**

0**011** 0000 | X | **FB_BRAKE_FACTOR** ➔ 0-7:1x,2x,3x,4x,6x,8x,16x,no braking; Default: 3

0000 **01**00 | X | **LOOP_GAIN**, 0-3:low,med,high,very high; Default: 1

0000 00**10** | X | **BEMF_GAIN**, 0-3 in LRA:5x,10x,20x,30x; Default: 2

---

- Brake Factor should be set prior calib. Trial and Error. Can influence stability
- Loop Gain should be set prior calib. Trial and Error. Can influence stability (high is unstable but fast...)
- back EMF Gain gets populated by calib (could be saved on raspi). 



### 0x1B – CONTROL1

**1**000 0000 | X | **STARTUP_BOOST** ➔ if 1: higher loop gain during overdrive; Default: 1

0**0**00 0000 | X | -

00**0**0 0000 | X | not important for RTP

000**1 0011** | X | **DRIVE_TIME**, initial guess for LRA drive time.

---

Drive Time:  If too low: startup time weak. If too high: instability

Use https://www.symbolab.com/: solve\:for\:x,\:0.5\cdot \frac{1000}{FREQUENCY}\:=\:x\:\cdot \:0.1\:+0.5

*G1040003D:* 24 (Frequency: 175Hz)

*G0832012:* 16 (Frequency: 235Hz)



### 0x1C – CONTROL2

**1**000 0000 | X | **BIDIR_INPUT** ➔ if 0: unidirectional, if 1: bidirectional, Default: 1

0**1**00 0000 | X | **BRAKE_STABILIZER** ➔ loop gain is reduced when braking ~ complete -> stable, Default: 1

00**11** 0000 | X | **SAMPLE_TIME** ➔ 0:150us, 1:200us, 2:250us, 3:300us, 

0000 **01**00 | X | BLANKING_TIME ➔ 0 or 1 – blanking before back EMF calculation – trial an error

0000 00**01** | X | IDISS_TIME ➔ 0 or 1 – ?

---

Sample, Blanking and IDISS can solve failed calib. Mostly trial an error. Poor documentation



### 0x1D – CONTROL3

... some unimportant registers

0000 **0**000 | X | **DATA_FORMAT_RTP** ➔ if 0: signed RTP input, if 1: unsigned

0000 0**0**00 | X | **LRA_DRIVE_MODE** ➔ update drive amplitude 0: once p cycle; 1: twice (more precise)

0000 00**0**0 | X | pwm stuff

0000 000**0** | X | **LRA_OPEN_LOOP** ➔ **0: auto-resonance** (1:open-loop mode)



### 0x1E – CONTROL4

... some unimportant registers

**00**00 0000 | X | -

00**10** 0000 | X | **AUTO_CAL_TIME** ➔ **0:150-350ms** (1-3 more ms) 

0000 0**0**00 | X | OTP_STATUS ➔ one time memory already written? (0: no, 1:yes)

0000 000**0** | X | OTP_PROGRAM ➔ programm OTP (see manual, no need here)

---

Auto Cal Time should be enough that motor can settle vibration with rated voltage 

*G1040003D:* 10ms 

*G0832012:* 50ms



### 0x21 – VOLTAGE MONITOR

**0000 0000** | X |  VBAT ➔ supply voltage at the VDD pin: V=VBAT × 5.6V / 255



### 0x22 – RESONANCE PERIOD

**0000 0000** | X |  VBAT ➔ LRA resonance period

---

Measurement of the LRA resonance period – when measuring you need to apply a waveform

LRA period (us) = LRA_Period × 98.46 μs







