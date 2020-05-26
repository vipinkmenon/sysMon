
#include "xsysmon.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "sleep.h"
#include "sdCard.h"
#include <stdio.h>

#define MAX_LOG_NUM 20

FIL* fptr;

static void SysMonInterruptHandler(void *CallBackRef);
static int SysMonSetupInterruptSystem(XScuGic* IntcInstancePtr,
				      XSysMon *SysMonPtr,
				      u16 IntrId );
int SysMonFractionToInt(float FloatNum);
int logNum=0;

char dataBuffer[1024];
char *dataPntr=dataBuffer;

XSysMon SysMonInst; 	  /* System Monitor driver instance */
XScuGic InterruptController; /* Instance of the XIntc driver. */

int main(void)
{

	int Status;
	XSysMon_Config *ConfigPtr;
	/*
	 * Initialize the SysMon driver.
	 */
	Status = SD_Init();

	if(Status != XST_SUCCESS)
		xil_printf("SD card init failed");

	fptr = openFile("logData.csv",'a');

	if(fptr == 0)
		printf("File opening failed\n\r");


	ConfigPtr = XSysMon_LookupConfig(XPAR_SYSMON_0_DEVICE_ID);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}
	XSysMon_CfgInitialize(&SysMonInst, ConfigPtr, ConfigPtr->BaseAddress);
	/*
	 * Set the sequencer in Single channel mode.
	 */
	XSysMon_SetSequencerMode(&SysMonInst, XSM_SEQ_MODE_SINGCHAN);
	/*
	 * Set the configuration registers for single channel continuous mode
	 * of operation for the Temperature channel.
	 */
	Status=  XSysMon_SetSingleChParams(&SysMonInst, XSM_CH_TEMP,
						FALSE, FALSE, FALSE);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	/*
	 * Setup the interrupt system.
	 */
	Status = SysMonSetupInterruptSystem(&InterruptController,
			&SysMonInst,XPAR_FABRIC_XADC_WIZ_0_IP2INTC_IRPT_INTR);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	/*
	 * Enable EOC interrupt
	 */
	XSysMon_IntrEnable(&SysMonInst, XSM_IPIXR_EOC_MASK);
	/*
	 * Enable global interrupt of System Monitor.
	 */
	XSysMon_IntrGlobalEnable(&SysMonInst);

	while(1);
}


static void SysMonInterruptHandler(void *CallBackRef)
{
	u32 IntrStatusValue;
	u16 TempRawData;
	float TempData;
	XSysMon *SysMonPtr = (XSysMon *)CallBackRef;
	/*
	 * Get the interrupt status from the device and check the value.
	 */
	XSysMon_IntrGlobalDisable(&SysMonInst);
	IntrStatusValue = XSysMon_IntrGetStatus(SysMonPtr);
	XSysMon_IntrClear(SysMonPtr, IntrStatusValue);
	logNum++;

	if (IntrStatusValue & XSM_IPIXR_EOC_MASK) {
		TempRawData = XSysMon_GetAdcData(&SysMonInst, XSM_CH_TEMP);
		TempData = XSysMon_RawToTemperature(TempRawData);
		printf("\r\nThe Current Temperature is %0.3f Centigrades. %d\r\n",TempData,logNum);
		sprintf(dataPntr,"%0.3f\n",TempData);
		dataPntr = dataPntr+8;

		if(logNum%10 == 0){
			xil_printf("Updating SD card...\n\r");
			writeFile(fptr, 80, (u32)dataBuffer);
			dataPntr = (char *)dataBuffer;
		}

		if(logNum == MAX_LOG_NUM){
			closeFile(fptr);
			SD_Eject();
			xil_printf("Safe to remove SD Card...\n\r");
			XSysMon_IntrGlobalDisable(&SysMonInst);
		}
		else{
			sleep(1);
			XSysMon_IntrGlobalEnable(&SysMonInst);
		}
	}

 }


static int SysMonSetupInterruptSystem(XScuGic* IntcInstancePtr,
				      XSysMon *SysMonPtr,
				      u16 IntrId )
{
	int Status;
	XScuGic_Config *IntcConfig;
	IntcConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, IntrId,
					0xA0, 0x3);
	Status = XScuGic_Connect(IntcInstancePtr, IntrId,
				 (Xil_ExceptionHandler)SysMonInterruptHandler,
				 SysMonPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}
	XScuGic_Enable(IntcInstancePtr, IntrId);

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XScuGic_InterruptHandler,(void *)IntcInstancePtr);
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}
