/*
 * houseKeeping.c
 *
 * Copyright (C) 2019, Universidade de Brasilia - FGA
 *
 * This file is part of Firmware_OBC.
 *
 * Firmware_OBC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Firmware_OBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FloripaSat-OBDH.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

 /**
 * \file houseKeeping.c
 *
 * \brief houseKeeping file
 *
 * \author Guilherme Lion�o
 *
 */

#include "houseKeeping.h"
#include <util/itoa.h>

////////////ADC//////////////////
volatile uint16_t cal30;
volatile uint16_t cal85;
volatile float calDifference;
volatile float tempC;
volatile float tempF;
//internal temperature sensor P5.5 (A0)
static uint16_t resultsBuffer[8];
////////////////////////////////////

extern QueueHandle_t xQueueIMU;
extern SemaphoreHandle_t semaphoreIMU;

void adcInit(void);
void getTemperature(int16_t *temperatureBuffer);

void *houseKeeping(void *pvParameters){

    ImuData imuData;
    MPU9250_initialize();
    int16_t ax,ay,az,gx,gy,gz,mx,my,mz;

    adcInit();
    memset(resultsBuffer, 0x00, 5 * sizeof(uint8_t));
    int16_t temperatureValue;


    while(1){


        MPU9250_getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);

        itoa(ax, &imuData.ax, DECIMAL);
        itoa(ay, &imuData.ay, DECIMAL);
        itoa(az, &imuData.az, DECIMAL);
        itoa(mx, &imuData.mx, DECIMAL);
        itoa(my, &imuData.my, DECIMAL);
        itoa(mz, &imuData.mz, DECIMAL);
        /*itoa(gx, &imuData.gx, DECIMAL);
        itoa(gy, &imuData.gy, DECIMAL);
        itoa(gz, &imuData.gz, DECIMAL);
*/
        obcData.imuData =imuData;

        getTemperature(&temperatureValue);
        ADC14_getMultiSequenceResult(resultsBuffer);

        obcData.internalTemperature = temperatureValue;
        obcData.obc_sensors[1] = resultsBuffer[1];
        obcData.obc_sensors[2] = resultsBuffer[2];
        obcData.obc_sensors[3] = resultsBuffer[3];
        obcData.obc_sensors[4] = resultsBuffer[4];
        /**/
        if(xSemaphoreTake(semaphoreIMU,200)){

            while(!xQueueSend( xQueueIMU, &imuData, 100)) ;

            xSemaphoreGive(semaphoreIMU);
        }

        vTaskDelay(100);
    }

    //vTaskDelete( NULL );

}

void getTemperature(int16_t *temperatureBuffer){

    int16_t adcValue;

    adcValue = ((ADC14_getResult(ADC_MEM0) - cal30) * 55);

    *temperatureBuffer = (adcValue / calDifference) + 30.0f;

}

void adcInit(void){

    /* Initializing ADC (MCLK/1/1) with temperature sensor routed */
    ADC14_enableModule();
    ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1,
                     ADC_TEMPSENSEMAP);

    /* Configuring GPIOs for Analog In */
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5,
            GPIO_PIN3 | GPIO_PIN2 | GPIO_PIN1| GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM7 (A0 - A5)  with repeat)
     * with internal 2.5v reference */
    ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM5, true);
    ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_INTBUF_VREFNEG_VSS,
            ADC_INPUT_A0, false);
    ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_INTBUF_VREFNEG_VSS,
            ADC_INPUT_A1, false);
    ADC14_configureConversionMemory(ADC_MEM2,
            ADC_VREFPOS_INTBUF_VREFNEG_VSS,
            ADC_INPUT_A2, false);
    ADC14_configureConversionMemory(ADC_MEM3,
            ADC_VREFPOS_INTBUF_VREFNEG_VSS,
            ADC_INPUT_A3, false);
    ADC14_configureConversionMemory(ADC_MEM4,
            ADC_VREFPOS_INTBUF_VREFNEG_VSS,
            ADC_INPUT_A4, false);
    ADC14_configureConversionMemory(ADC_MEM5,
            ADC_VREFPOS_INTBUF_VREFNEG_VSS,
            ADC_INPUT_A5, false);

    /* Configuring the sample/hold time for 192 */
    ADC14_setSampleHoldTime(ADC_PULSE_WIDTH_192,ADC_PULSE_WIDTH_192);

    ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    REF_A_enableTempSensor();
    REF_A_setReferenceVoltage(REF_A_VREF2_5V);
    REF_A_enableReferenceVoltage();

    //data to calibrate the temperature sensor
    cal30 = SysCtl_getTempCalibrationConstant(SYSCTL_2_5V_REF,
            SYSCTL_30_DEGREES_C);
    cal85 = SysCtl_getTempCalibrationConstant(SYSCTL_2_5V_REF,
            SYSCTL_85_DEGREES_C);
    calDifference = cal85 - cal30;

    /* Triggering the start of the sample */
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();
}
