# Blood-Pressure-Machine-Using-Oscillometric-Technique

 * **Algorithm** : Find the largest +ve slope from all slopes of all peaks. This is eqivalent to finding the strongest pump of
              blood by the heart against the pressure in the cuff.
              This largest +ve slope is in the high frequency oscillation (ie peaks) range that occurs during pump deflation. 
              Corresponding to this slope is the patients Mean Arterial Pressure (ie MAP).
              The SYSTOLIC pressure value will be the closest slope value to 0.5 * MAP in the left part of MAP.
              The DIASTOLIC pressure value will be the closest slope value to 0.8 * MAP in the right part of MAP.
              The HEART RATE is the count of number of peaks in each second, averaged over entire time between
              systolic and diastolic pressure.



* **Youtube Demonstration Video** : https://youtu.be/zGHvD9UG5J0

* **Development Environment** :
  * Honeywell MPRLS0300YG00001BB Pressure Sensor
  * STM32F4DISCOVERY ARM based Mircrocontroller
  * MBED OS
  * PLATFORMIO developemnt platform

* For any queries / feedback / constructive criticism / collaborations on this or any other of my projects you are most welcome to drop me an email : ssc9983@nyu.edu

* **NOTE** : Feel free to use this code for your development but before doing so please go through the License above.
