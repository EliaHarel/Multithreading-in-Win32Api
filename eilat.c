#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <string.h> 

#define BUFFER_SIZE 55
#define MAX_SLEEP_TIME 3000 //Miliseconds (3 second)
#define MIN_SLEEP_TIME 5 // Miliseconds
#define True 1
#define False 0
#define Weight 1
#define Crane 0
#define MIN_CARGO_W 5
#define MAX_CARGO_W 55

int numberOfPuking;
int numInsideUnloading;
int nextCrane;
int flag = 0;
HANDLE mutex;
CHAR buffer[BUFFER_SIZE];
SYSTEMTIME timeToPrint;

//pipe arguments
HANDLE ReadHandle, WriteHandle;
DWORD read, written;
STARTUPINFO si;
PROCESS_INFORMATION pi;

//vessels arguments
HANDLE* semVessel;
HANDLE* vessels;
DWORD WINAPI Vessel(PVOID);
DWORD ThreadId;
int* vesselIDarr;
int* cranelIDarr;
int** vesselCargoArr;
int numOfVes = 0;
int vID;

//crane arguments
HANDLE* cranes;
HANDLE* semCranesADT;
DWORD WINAPI Cranes(PVOID); //Thread function for each vessel
HANDLE mutexCrane;
int* ArrCranes;
int* cranesId;
int numOfCranes = 0;
int* vesselCrane;

// barrier arguments
int* barrierQ;
int* vesselcarg;
int in, out;
HANDLE barrierTrd;
DWORD WINAPI Barrier(PVOID); //Thread function for barrier
HANDLE mutexBarrier;

//Functions declartion
int initGlobalData();
int checkPrime(int);
int calcSleepTime();
int randNum();
int createVessel(int);
int createCrane(int);
int createBarrier(int, int);
int cargoWeight();
int exitBarrier(int);
int cleanupGlobalData();
boolean Divisor(int);
boolean emptyUploadingQuay();
char* timePrinting();
void backToSuez(int);
void enterBarrier(int);
void unPacking(int);


int main(VOID)
{
	ReadHandle = GetStdHandle(STD_INPUT_HANDLE);
	WriteHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	/* check the num of vessels */
	if (ReadFile(ReadHandle, buffer, BUFFER_SIZE, &read, NULL)) {
		numOfVes = atoi(buffer);
		flag = checkPrime(numOfVes);
		if (flag == 0) {
			sprintf(buffer, "%d", flag);  
		}
		else {
			sprintf(buffer, "%d", flag);
			fprintf(stderr, "is prime");
		}
	}
	else
		fprintf(stderr, "Child: Error reading from pipe\n");


	if (!WriteFile(WriteHandle, buffer, BUFFER_SIZE, &written, NULL))
		fprintf(stderr, "Error writing to pipe\n");

	//If prime, finish the program
	if (flag) exit(0);

	int cnt = 0;
	
	//Initializa global data after confirmation from Eilat
	if (initGlobalData() == False)
	{
		fprintf(stderr, "initGlobalData()::Unexpected Error in Global Semaphore Creation\n");
		return 1;
	}


	int i = 0;
	while (ReadFile(ReadHandle, buffer, BUFFER_SIZE, &read, NULL)) {

		if (strcmp(buffer, "") != 0) {
			vID = atoi(buffer);
			createVessel(vID);
			i++;
			if (i == numOfVes)
				break;
		}

	}


	WaitForMultipleObjects(numOfVes, vessels, TRUE, INFINITE);
	fprintf(stderr, "[%s] Eilat Port: All Vessel Threads are done\n", timePrinting());

	WaitForSingleObject(si.hStdError, INFINITE);

	if(cleanupGlobalData())
		fprintf(stderr,"[%s] Eilat Port: Exiting....\n", timePrinting());

	exit(0);
	return 0;
}

//generic function to check if number of vessels from haifa is prim
int checkPrime(int n) {

	for (int i = 2; i * i <= n; i++) {
		if (n % i == 0) return 0;
	}
	return 1;
}

//generic function to initial all mutex\semaphore and allocated memory to variable
int initGlobalData()
{
	numOfCranes = randNum();
	while (Divisor(numOfCranes) == FALSE)
	{

		numOfCranes = randNum();
	}

	vessels = (HANDLE*)malloc(numOfVes * sizeof(HANDLE));
	semVessel = (HANDLE*)malloc(numOfVes * sizeof(HANDLE));
	cranes = (HANDLE*)malloc(numOfCranes * sizeof(HANDLE));


	vesselIDarr = (int*)malloc(numOfVes * sizeof(int));
	cranelIDarr = (int*)malloc(numOfCranes * sizeof(int));
	barrierQ = (int*)malloc(numOfVes * sizeof(int));
	vesselCrane = (int*)malloc(numOfCranes * sizeof(int));
	vesselcarg = (int*)malloc(numOfCranes * sizeof(int));
	for (int i = 0; i < numOfCranes; i++) {
		vesselCrane[i] = -1;
	}
	vesselCargoArr = (int**)malloc(numOfVes * sizeof(int*));
	for (int i = 0; i < numOfCranes; i++)
	{
		vesselCargoArr[i] = (int*)malloc(2 * sizeof(int));
	}
	cranesId = (int*)malloc(numOfCranes * sizeof(int));
	ArrCranes = (int*)malloc(numOfCranes * sizeof(int));
	semCranesADT = (HANDLE*)malloc(numOfCranes * sizeof(HANDLE));


	if ((vessels || vesselIDarr || semVessel || cranes || vesselCargoArr || cranesId || ArrCranes || semCranesADT) == NULL) {
		fprintf(stderr, "memory error!!!");
		return -1;
	}

	mutex = CreateMutex(NULL, FALSE, NULL);
	mutexCrane = CreateMutex(NULL, FALSE, NULL);
	mutexBarrier = CreateMutex(NULL, FALSE, NULL);

	if ((mutex || mutexCrane || mutexBarrier) == NULL)
	{

		return False;
	}

	for (int i = 0; i < numOfVes; i++)
	{
		semVessel[i] = CreateSemaphore(NULL, 0, 1, NULL);
		if (semVessel[i] == NULL)
		{
			return False;
		}
	}
	for (int i = 0; i < numOfCranes; i++)
	{
		semCranesADT[i] = CreateSemaphore(NULL, 0, 1, NULL);
		if (semCranesADT[i] == NULL)
		{
			return False;
		}
	}

	//Initialise cranes Threads
	for (int i = 1; i < numOfCranes + 1; i++)
	{
		createCrane(i);
	}

	Sleep(calcSleepTime());

	//create barrier
	if (!createBarrier(numOfVes, numOfCranes)) {
		fprintf(stderr, "ERROR CREATING BARRIER - FIINISH PROGRAM");
		exit(0);
	}
	fprintf(stderr, "[%s] numbers of cranes created -  %d\n", numOfCranes);


	return True;
}

//Close all global semaphore handlers - after all vessels Threads finish.
int cleanupGlobalData()
{
	int i;

	if (!CloseHandle(mutex) && !CloseHandle(mutexBarrier) && !CloseHandle(mutexCrane)) {
		fprintf(stderr, "cleanupGlobalData::close mutex failed!");
		return False;
	}
	for (i = 0; i < numOfVes; i++)
	{

		if (!CloseHandle(semVessel[i])) {
			fprintf(stderr, "cleanupGlobalData::close vessels semaphores failed!");
			return False;

		}
	}

	for (i = 0; i < numOfCranes; i++)
	{

		if (!CloseHandle(semCranesADT[i])) {
			fprintf(stderr, "cleanupGlobalData::close cranes semaphores failed!");
			return False;
		}
	}
	for (i = 0; i < numOfCranes; i++)
	{

		if (!CloseHandle(cranes[i])) {
			fprintf(stderr, "cleanupGlobalData::close cranes semaphores failed!");
			return False;
		}
	}
	for (i = 0; i < numOfVes; i++)
	{

		if (!CloseHandle(vessels[i])) {
			fprintf(stderr, "cleanupGlobalData::close cranes semaphores failed!");
			return False;
		}
	}


	//close all variable that getting memory allocated
	free(semCranesADT);
	free(semVessel);
	free(vessels);
	free(cranes);
	free(cranelIDarr);
	free(barrierQ);
	free(vesselCrane);
	free(vesselcarg);
	free(vesselCargoArr);
	free(cranesId);
	free(ArrCranes);


	return True;
}

//generic function that change int to string
char* inttostr(int n) {
	char* result;
	if (n >= 0)
		result = malloc((size_t)floor(log10(n)) + 2);
	else
		result = malloc((size_t)floor(log10(n)) + 3);

	sprintf(result, "%d", n);
	return result;
}

//function that send the vessels frome Eilat to suez
void backToSuez(int id)
{

	//Write to the pipe
	WaitForSingleObject(mutex, INFINITE);
	Sleep(calcSleepTime());
	int sizeBuf = sizeof(inttostr(id));

	if (!WriteFile(WriteHandle, inttostr(id), sizeBuf, &written, NULL))
		fprintf(stderr, "Error writing to pipe\n");

	if (!ReleaseMutex(mutex))
		fprintf(stderr, "EilatToSuez::Unexpected error mutex.V()\n");

}

//generic function to create vessel thread by unique ID
int createVessel(int vID) {

	vesselIDarr[vID - 1] = vID;


	vessels[vID - 1] = CreateThread(NULL, 0, Vessel, &vesselIDarr[vID - 1], 0, &ThreadId);
	if (vessels[vID - 1] == NULL)
	{
		fprintf(stderr, "main::Unexpected Error in Vessel %d Creation\n", vID);
		return 1;
	}

	return 0;
}

//generic function to create crane thread by crane ID
int createCrane(int id) {

	cranesId[id -1] = id;
	ArrCranes[id -1] = -1;

	cranes[id -1] = CreateThread(NULL, 0, Cranes, &cranesId[id-1], 0, &ThreadId);
	fprintf(stderr, "[%s] crane %d - created \n", timePrinting(),id);

	if (cranes[id - 1] == NULL)
	{
		fprintf(stderr, "main::Unexpected Error in cranes %d Creation\n", id);
		return 1;
	}

	return 0;
}

//generic function to create the barrier
int createBarrier(int numOfVes, int numOfCrane) {
	barrierTrd = CreateThread(NULL, 0, Barrier, NULL, 0, NULL);
	return 1;
}

//thread function for Barrier thread
DWORD WINAPI Barrier(PVOID Param)
{

	while (out < numOfVes) {
		WaitForSingleObject(mutexBarrier, INFINITE);
		if (numInsideUnloading == 0 && (in - out) >= numOfCranes)
		{
			nextCrane = 0;
			for (int i = 0; i < numOfCranes; i++)
			{
				fprintf(stderr, "[%s] Vessel %d - is now Entering to the ADT (unloading Queue)\n", timePrinting(), barrierQ[out]);
				Sleep(calcSleepTime());
				numInsideUnloading++;
				if (!ReleaseSemaphore(semVessel[barrierQ[out++] - 1], 1, NULL))
					fprintf(stderr,"Barrier::Unexpected error semVessel.v()\n");
			}
		}
		if (!ReleaseMutex(mutexBarrier))
			fprintf(stderr, "Barrier::Unexpected error mutexBarrier.v()\n");
	}
	return 0;
}

//thread function for crane thread by unique ID
DWORD WINAPI Cranes(PVOID Param)
{
	int craneId = *(int*)Param;


	while (True) {
		WaitForSingleObject(semCranesADT[craneId - 1], INFINITE);

		WaitForSingleObject(mutexBarrier, INFINITE);

		unPacking(craneId);



		if (numberOfPuking == numOfVes) {
			break;
		}
		if (!ReleaseMutex(mutexBarrier))
			fprintf(stderr, "EilatToSuez::Unexpected error mutexBarrier.v()\n");
	}


	return 0;
}

//thread function for vessels thread by unique ID
DWORD WINAPI Vessel(PVOID Param)
{
	int vesselId = *(int*)Param;
	fprintf(stderr, "[%s] Vessel %d - arrived @ Eilat Port\n", timePrinting(), vesselId);
	enterBarrier(vesselId);

	WaitForSingleObject(semVessel[vesselId - 1], INFINITE);
	Sleep(calcSleepTime());

	exitBarrier(vesselId);
	WaitForSingleObject(semVessel[vesselId - 1], INFINITE);
	Sleep(calcSleepTime());
	fprintf(stderr, "[%s] Vessel %d - exit ADT (uploading quay)\n", timePrinting(), vesselId);
	fprintf(stderr, "[%s] Vessel %d - entering Canal: Red Sea ==> Med. Sea  \n", timePrinting(), vesselId);
	Sleep(calcSleepTime());
	backToSuez(vesselId);

	return 0;
}

//function for enter vessel for waiting in FIFO at the thread barrier
void enterBarrier(int vID) {

	WaitForSingleObject(mutexBarrier, INFINITE);
	fprintf(stderr, "[%s] Vessel %d - is now waiting at the Barrier\n", timePrinting(), vID);
	barrierQ[in++] = vID;
	if (!ReleaseMutex(mutexBarrier))
		fprintf(stderr, "EilatToSuez::Unexpected error mutexBarrier.v()\n");
}

//function for exiting vessels thread from barrier to the uploading quay
int exitBarrier(int vID) {

	WaitForSingleObject(mutexBarrier, INFINITE);

	vesselCrane[nextCrane] = vID;
	vesselcarg[nextCrane] = cargoWeight();

	Sleep(calcSleepTime());

	if (!ReleaseSemaphore(semCranesADT[nextCrane++], 1, NULL))
		fprintf(stderr,"exitBarrier::Unexpected error semCranesADT.v()\n");

	if (!ReleaseMutex(mutexBarrier))
		fprintf(stderr, "exitBarrier::Unexpected error mutexBarrier.v()\n");

	return 0;
}

//function that getting crane id to unpacing vessel cargo
void unPacking(int craneID) {


	int vesID = vesselCrane[craneID - 1];
	fprintf(stderr, "[%s] crane  %d - starts unpacking vessel %d with weight of %d tones \n", timePrinting(), craneID, vesID, vesselcarg[craneID - 1]);
	Sleep(calcSleepTime());
	fprintf(stderr, "[%s] crane  %d - finish unpacking vessel %d with weight of %d tones\n", timePrinting(), craneID, vesID, vesselcarg[craneID - 1]);

	numInsideUnloading--;
	numberOfPuking++;

	if (!ReleaseSemaphore(semVessel[vesID - 1], 1, NULL))
		fprintf(stderr,"unPacking::Unexpected error semVessel.v()\n");



}

//boolean function to check if the qay is empty
boolean emptyUploadingQuay() {

	for (int i = 0; i < numOfCranes; i++)
	{
		if (ArrCranes[i] != -1)
			return FALSE;
	}

	return TRUE;
}

//generic function to randomise a cargo weight between MIN_CARGO_W and MAX_CARGO_W
int cargoWeight() {
	srand((unsigned int)time(NULL));
	return(MIN_CARGO_W + rand() % (MAX_CARGO_W - MIN_CARGO_W + 1));
}

//generic function to randomise a Sleep time between 1 and MAX_SLEEP_TIME msec
int calcSleepTime()
{
	srand((int)time(NULL));
	return(rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME + 1) - MIN_SLEEP_TIME);
}

//generic function to randomise a number
int randNum() {
	srand((int)time(NULL));
	return(rand() % numOfVes + 1);
}

//generic function to print system time information
char* timePrinting()
{
	GetLocalTime(&timeToPrint);
	static char currentLocalTime[20];
	if (timeToPrint.wHour < 12)	// before midday
		sprintf(currentLocalTime, "%02d:%02d:%02d am", timeToPrint.wHour, timeToPrint.wMinute, timeToPrint.wSecond);

	else	// after midday
		sprintf(currentLocalTime, "%02d:%02d:%02d pm", timeToPrint.wHour - 12, timeToPrint.wMinute, timeToPrint.wSecond);
	return currentLocalTime;
}

//generic boolean function to randomise divisor number
boolean Divisor(int randNum) {
	if ((numOfVes % randNum != 0) || (randNum == numOfVes) || (randNum == 1))
		return False;
	else
		return True;
}

