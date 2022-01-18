#include "sdCard.h"


static FATFS  fatfs;

int SD_Init()
{
	FRESULT rc;
	TCHAR *Path = "0:/";
	rc = f_mount(&fatfs,Path,0);
	if (rc) {
		xil_printf(" ERROR : f_mount returned %d\r\n", rc);
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}

int SD_Eject()
{
	FRESULT rc;
	TCHAR *Path = "0:/";
	rc = f_mount(0,Path,1);
	if (rc) {
		xil_printf(" ERROR : f_mount returned %d\r\n", rc);
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}



int ReadFile(FIL *fil, u32 DestinationAddress)
{

	FRESULT rc;
	UINT br;
	u32 file_size;

	file_size = fil->fsize;
	rc = f_lseek(fil, 0);
	if (rc) {
		xil_printf(" ERROR : f_lseek returned %d\r\n", rc);
		return XST_FAILURE;
	}
	rc = f_read(fil, (void*) DestinationAddress, file_size, &br);
	if (rc) {
		xil_printf(" ERROR : f_read returned %d\r\n", rc);
		return XST_FAILURE;
	}
	Xil_DCacheFlush();
	return file_size;
}

u32 closeFile(FIL* fptr){
	FRESULT rc; // FRESULT variable
	rc = f_close(fptr);
	if (rc) {
		xil_printf(" ERROR : f_close returned %d\r\n", rc);
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}


FIL* openFile(char *FileName,char mode){
	static FIL fil; // File instance
	FRESULT rc; // FRESULT variable
	if(mode == 'r')
		rc = f_open(&fil, FileName, FA_READ);
	else if(mode == 'w'){
		rc = f_open(&fil, (char *)FileName, FA_CREATE_NEW | FA_WRITE); //f_open
		if(rc != FR_OK){ //file already exists
			rc = f_unlink(FileName);//delete the file;
			rc = f_open(&fil, (char *)FileName, FA_CREATE_NEW | FA_WRITE); //f_open
		}
	}
	else if(mode == 'a'){
		rc = f_open(&fil, (char *)FileName, FA_OPEN_ALWAYS | FA_WRITE); //f_open
		if(rc != FR_OK){ //file doesn't exists
			rc = f_open(&fil, (char *)FileName, FA_CREATE_NEW | FA_WRITE); //f_open
		}
		else
			rc = f_lseek(&fil, fil.obj.objsize);
	}
	if (rc) {
		xil_printf(" ERROR : f_open returned %d\r\n", rc);
		return (FIL*)0;
	}
	return &fil;
}

int writeFile(FIL* fptr, u32 size, u32 SourceAddress){
	UINT btw;
	FRESULT rc; // FRESULT variable
	//xil_printf("Writing file %0x size %0x address %0d",fptr->fptr,size,SourceAddress);
	rc = f_write(fptr,(const void*)SourceAddress,size,&btw);
	if (rc) {
		xil_printf(" ERROR : f_write returned %d\r\n", rc);
		return XST_FAILURE;
	}
	return btw;
}


