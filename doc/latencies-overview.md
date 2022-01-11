# Notes on Latencies of the System



Latency is crucial in Sensory Substitution Systems. These are some notes about the latency of the system. Consider them as notes of thought rather then calibrated measurements.



## Total Latency

To measure the latency between **a change in the environment** and the **change in the vibration** motors, I built a setup in which I had a stressed plastic part (outside the field of view of the camera) snapping against an object (inside the field of view). A microphone mounted on the corresponding vibration motor recorded the impact noise (very short) and finally the vibration of the motor (increasing). Neglecting the transit time of the sound (20cm → approx. 0.5ms) to the microphone and any inaccuracies between the entry of the object into the field of view, I got measurement results **between 40-60ms.**

In this default setting, the camera runs at 25 frames per second, bleaches 450 us long and, according to the data sheet, needs a total of 4.8 ms to produce an image. So depending on when the object comes into view, it can take ~5 to 45 ms until the image with the object is ready. The unfolding app adds only about 3 ms to the chain, of which about 50% is spent writing the motor driver registers. According to the data sheet, the motors used have a rising time of approx. 10 ms up to 50% of the power (and a little more to 100%, let’s just say 20ms). All this together would result in a calculated latency of 28 - 68 ms with an average of 48 ms. **This corresponds approximately to the measurement results.**
