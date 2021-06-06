/* RTES Spring 2021 Embedded Challenge
 *
 * Oscillometric Blood Pressure Machine
 * 
 * Author             : Sahil Chitnis, N10986573, ssc9983
 * Date of Submission : 05/23/2021
 * 
 * Method Used        : SLOPE METHOD
 * 
 * Language Used      : C
 * 
 * Algorithm : Find the largest +ve slope from all slopes of all peaks. This is eqivalent to finding the strongest pump of
 *             blood by the heart against the pressure in the cuff.
 *             This largest +ve slope is in the high frequency oscillation (ie peaks) range that occurs during pump deflation. 
 *             Corresponding to this slope is the patients Mean Arterial Pressure (ie MAP).
 *             The SYSTOLIC pressure value will be the closest slope value to 0.5 * MAP in the left part of MAP.
 *             The DIASTOLIC pressure value will be the closest slope value to 0.8 * MAP in the right part of MAP.
 *             The HEART RATE is the count of number of peaks in each second, averaged over entire time between
 *             systolic and diastolic pressure.
 *
 * License. : GNU General Public License v3.0 (Detailed license under same directoy)
 */

// ********************************* Header Files Includes **********************************************

#include "mbed.h"
#include "USBSerial.h"

// ********************************* Object Declarations / Instantiations ********************************

// USBSerial Object
USBSerial serial;

// I2C Object
I2C master(I2C_SDA, I2C_SCL);

// Timer Object
Timer t;

// ************************************ FUNCTION calculateHeartRate *************************************
//
//  @Brief Calculate Heart Rate
//         Algorithm : Find total average number of waves (peaks) per time slice between systolic and diastolic
//                     Since each peak is equivalent to a heart beat.
//
void calculateHeartRate(int bestSysSlopeIndex, int bestDiaSlopeIndex, float *slopeArray, float *timeArray, int *heartRate)
{
    int hrCounter = 0;

    for (int loop5 = bestSysSlopeIndex; loop5 <= bestDiaSlopeIndex; loop5++)
    {
        if (slopeArray[loop5] >= 0.0)
            hrCounter++;
    }
    (*heartRate) = ((hrCounter / (timeArray[bestDiaSlopeIndex] - timeArray[bestSysSlopeIndex])) * 60);
}

// ************************************ FUNCTION calculateDiastolic *************************************
//
// @Brief  Calculate Diastolic pressure
//         Algorithm : Slope value nearest to (0.8 * MAP) to the right side of MAP in the curve
//
void calculateDiastolic(float MAP, int mapIndex, int endIndex, float *slopeArray, int *bestDiaSlopeIndex)
{

    float diaSlopeThreshold = 0.8 * MAP;
    float slopeDiff = 0;
    float minDiffSlope = INT32_MAX;

    int loop4 = 0;

    for (loop4 = mapIndex + 1; loop4 < endIndex; loop4++)
    {
        if ((slopeArray[loop4] >= 0.0) && (slopeArray[loop4] < diaSlopeThreshold))
        {
            slopeDiff = diaSlopeThreshold - slopeArray[loop4];
            if (slopeDiff < minDiffSlope)
            {
                minDiffSlope = slopeDiff;
                (*bestDiaSlopeIndex) = loop4 + 1;
            }
        }
    }
}

// ************************************ FUNCTION calculateSystolic *************************************
//
// @Brief  Calculate Systolic pressure
//         Algorithm : Slope value nearest to (0.5 * MAP) to the left side of MAP in the curve
//
void calculateSystolic(float MAP, int mapIndex, float *slopeArray, int *bestSysSlopeIndex)
{
    float sysSlopeThreshold = 0.5 * MAP;
    float slopeDiff = 0;
    float minDiffSlope = (float)INT32_MAX;

    int loop3 = 0;

    for (loop3 = 0; loop3 < mapIndex - 1; loop3++)
    {
        if ((slopeArray[loop3] >= 0.0) && (slopeArray[loop3] < sysSlopeThreshold))
        {
            slopeDiff = sysSlopeThreshold - slopeArray[loop3];
            if (slopeDiff < minDiffSlope)
            {
                minDiffSlope = slopeDiff;
                (*bestSysSlopeIndex) = loop3 + 1;
            }
        }
    }
}

//
// ************************************ FUNCTION 4 *************************************
//
// @Brief  Calculate Maximum +ve slope in high frequency range as MAP
//
void calculateMAP(float *pressureArray, int endIndex, float *timeArray, float *slopeArray, float *MAP, int *mapIndex)
{
    int loop1 = 0;
    float pressureDiff = 0.0;
    float timeDiff = 0;

    for (loop1 = 1; loop1 < endIndex; loop1++)
    {
        pressureDiff = pressureArray[loop1] - pressureArray[loop1 - 1];
        timeDiff = (timeArray[loop1] - timeArray[loop1 - 1]);

        // Calculate Slope
        if (timeDiff != 0.000000)
            slopeArray[loop1 - 1] = (pressureDiff / timeDiff);

        // Calculate Max +ve Slope
        if (slopeArray[loop1 - 1] > (*MAP))
        {
            (*MAP) = slopeArray[loop1 - 1];
            (*mapIndex) = loop1;
        }
    }
}

//
// ************************************ FUNCTION MAIN *************************************
//
int main()
{
    //
    // Step0 : Variable declarations / initializations
    //
    volatile int i;
    int i2cSensorAddressToWrite = (0x18 << 1);
    int i2cSensorAddressToRead = 0x31;
    const char i2cOutputCommand[] = {0xAA, 0x00, 0x00};
    char sensorPressureOutput[4] = {0};
    float outputFromSensor = 0.0;
    float oMin = 419430.4;
    float oMax = 3774873.6;
    float pMin = 0.0;
    float pMax = 300.0;
    float pressure = 0.0;
    float slopeArray[1000] = {1.0};
    float MAP = 0.0;
    int mapIndex = 0;
    char *pressureDropRemark = "";

    float pressure_old = 0.0;
    int counter = 0;
    float pressureArray[1000];
    float timeArray[1000];
    char patientName[1000];
    int bestSysSlopeIndex = 0;
    int bestDiaSlopeIndex = 0;
    int heartRate = 0;
    float pressure_change;
    char accurate[100] = "Deflation Rate is Accurate";
    char fast[100] = "Deflation Rate is Fast, Please slow down";
    char slow[100] = "Deflation Rate is Slow, Please speed up";

    char sensorStatus = '0';
    bool increase = true, decrease = false;

    // Welcome Prints and User Interface

    serial.printf("\n ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ************ WELCOME TO RTES SPRING21 EMBEDDED CHALLENGE ************ \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");

    serial.printf(" Please Enter Patient Name below: \n");

    serial.scanf("%s", patientName);

    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ******************                  WELCOME %s     *************** \n", patientName);
    serial.printf(" ****** START PUMPING CUFF TO MEASURE YOUR BLOOD PRESSURE ***********\n");

    // Start Device Timer
    t.start();

    // Poll for pressure output from Honeywell Sensor
    while (1)
    {
        // Pressure is recorded during decrease 150mmHg onwards.
        if (pressure > 151)
            increase = false;

        // Decreases 150mmHg onwards.
        if (!increase && pressure < 151)
            decrease = true;

        // Data reading from sensor stops at 30mmHg
        if (pressure < 30 && decrease)
            break;

        //
        // Step 1) Send I2C output measurement command to the sensor
        //
        // Details : 1.1) Command of 0xAA, 0x00, 0x00
        //           1.2) Address of sensor is 0x18 < 1
        //
        i = master.write(i2cSensorAddressToWrite, i2cOutputCommand, 3);

        //
        // Step 2) Read Status from sensor
        //
        // Details : 2.1) Wait until the busy flag in the Status Byte Clears
        //
        master.read(i2cSensorAddressToRead, &sensorStatus, 1);
        while (((sensorStatus & 0x20) >> 5) == 0x1)
        {
            master.read(i2cSensorAddressToRead, &sensorStatus, 1);
            wait_ms(5);
        }

        //
        // Step 3) Read 24bit pressure output coming from sensor
        //
        i = master.read(i2cSensorAddressToWrite, sensorPressureOutput, 4);

        //
        // Step 4) ououtputFromSensortput = (float)((sensorPressureOutput[1] << 16) | (sensorPressureOutput[2] << 8) | (sensorPressureOutput[3]));
        // Details : 4.1) sensorPressureOutput[1] : 23:16, sensorPressureOutput[2] : 15:8, sensorPressureOutput[3] : 7:0
        //
        outputFromSensor = (float)((sensorPressureOutput[1] << 16) | (sensorPressureOutput[2] << 8) | (sensorPressureOutput[3]));
        //
        // Step 5) Pressure formula as per datasheet.
        //
        pressure = (((outputFromSensor - oMin) * (pMax - pMin)) / (oMax - oMin)) + pMin;

        //
        // Step 6) Monitor pressure drop
        //
        //monitorPressureDrop(pressure_old, pressure, &pressureDropRemark);

        //
        // Assumption is that since the sampling rate is 500ms, a pressure change of
        // 0.5 - 1.75mmHg equals 1 - 3.5mmHg drop per sec. I have assumed a smaller drop rate since
        // a drop rate of 3.5+ mmHg results in fewer peaks / heart beat readings, making the data
        // not good enough to calculate blood pressure.
        //
        pressure_change =  pressure_old - pressure;
        if (pressure_change > 0.8 && pressure_change < 2.0)
        {
            pressureDropRemark = accurate;
        }
        else if (pressure_change > 2.0)
        {
            pressureDropRemark = fast;
        }
        else
        {
            pressureDropRemark = slow;
        }
        int time_ms = t.read_ms();

        //
        // Step 7) Store pressure and array readings only during cuff deflation below 150 mmHg
        //
        if (decrease)
        {
            serial.printf("Pressure is %.2f at Time %d  | Changed by %.2f \n", pressure, (time_ms / 1000), (pressure - pressure_old));

            pressureArray[counter] = pressure;
            timeArray[counter] = time_ms / 1000;
            counter++;

        }
        else
        {
            // Print drop rate only during decrease between highest pressure point to 150mmHg since after that it will be difficult
            //  to assess the drop rate as it randomly varies due to blood pressure in the arteries.
            if (increase)
                serial.printf("Pressure is %.2f at Time %d  | Changed by %.2f \n",  pressure, (time_ms / 1000), (pressure - pressure_old));
            else
                serial.printf("Pressure is %.2f at Time %d | Changed by %.2f | %s \n", pressure, (time_ms / 1000), (pressure - pressure_old), pressureDropRemark);
        }

        //
        // Step 8) Store current pressure to variable pressure_old before taking new reading
        //
        pressure_old = pressure;

        wait_ms(500);
    }

    // Stop device timer
    t.stop();

    // Compute Patient's MAP - Mean Arterial Pressure
    calculateMAP(pressureArray, counter, timeArray, slopeArray, &MAP, &mapIndex);

    // Compute Patient's Systolic Pressure
    calculateSystolic(MAP, mapIndex, slopeArray, &bestSysSlopeIndex);

    // Compute Patient's Diastolic Pressure
    calculateDiastolic(MAP, mapIndex, counter, slopeArray, &bestDiaSlopeIndex);

    // Compute Patient's HeartRate
    calculateHeartRate(bestSysSlopeIndex, bestDiaSlopeIndex, slopeArray, timeArray, &heartRate);

    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" %s's SYSTOLIC PRESSURE IS %.2f and DIASTOLIC PRESSURE IS %.2f \n", patientName, pressureArray[bestSysSlopeIndex], pressureArray[bestDiaSlopeIndex]);
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" More information for %s's benefit", patientName);
    serial.printf(" %s's MEAN ARTERIAL PRESSURE IS %.2f \n", patientName, pressureArray[mapIndex]);
    serial.printf(" ******************* %s's HEART RATE IS %d beats per min ***************************\n", patientName, heartRate);
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ********************************************************************* \n");
    serial.printf(" ******************* THANKn YOU %s for using my DEVICE *************************** \n", patientName);
    serial.printf(" ******************* DEVICE MADE BY SAHIL CHITNIS *************************** \n");
}

// ************************************************* THE END ***********************************************
