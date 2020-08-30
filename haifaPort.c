#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

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

int numVes;
HANDLE mutex;
HANDLE* semVessel;
HANDLE* vessels;
int* vesselID;
DWORD WINAPI Vessel(PVOID); //Thread function for each vessel
DWORD ThreadId;

TCHAR ProcessName[256];
HANDLE StdInRead, StdInWrite;    /* pipe for writing parent to child */
HANDLE StdOutRead, StdOutWrite;  /* pipe for writing child to parent */
STARTUPINFO si;
PROCESS_INFORMATION pi;
char buffer[BUFFER_SIZE];
SYSTEMTIME timeToPrint;
DWORD written, read;

//Functions declartion
int initGlobalData();
void cleanupGlobalData();
char* inttostr(int n);
int calcSleepTime(); //generic function to randomise a Sleep time between 1 and MAX_SLEEP_TIME msec
void toSuez(int id);
char* timePrinting();


int main(int argc, char* argv[])
{
	//check if the number the client type is positive
	if (argc <= 1) {
		printf("You did not feed me arguments, I will die now :( ...");
		exit(1);
	}  //otherwise continue on our merry way....
	numVes = atoi(argv[1]);
	if (numVes >= 51 || numVes <= 2) {
		printf("Ships number should be between 2 to 51!!!! ----> ending process");
		exit(1);
	}


	/* set up security attributes so that pipe handles are inherited */
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL,TRUE };

	ZeroMemory(&pi, sizeof(pi));

	/* create the pipe */
	if (!CreatePipe(&StdInRead, &StdInWrite, &sa, 0)) {
		fprintf(stderr, "Create Pipe Failed\n");
		return 1;
	}

	/* create second the pipe */
	if (!CreatePipe(&StdOutRead, &StdOutWrite, &sa, 0)) {
		fprintf(stderr, "Create second Pipe Failed\n");
		return 1;
	}

	/* establish the START_INFO structure for the child process */
	GetStartupInfo(&si);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	/* redirect the standard input to the read end of the pipe */
	si.hStdInput = StdInRead;
	si.hStdOutput = StdOutWrite;
	si.dwFlags = STARTF_USESTDHANDLES;

	/* we do not want the child to inherit the write end of the pipe */
	//SetHandleInformation(StdInWrite, HANDLE_FLAG_INHERIT, 0);
	wcscpy(ProcessName, L"..\\..\\EilatPort\\Debug\\EilatPort.exe");

	// Start the child process.
	if (!CreateProcessW(NULL,
						ProcessName,   // No module name (use command line).
						NULL,             // Process handle not inheritable.
						NULL,             // Thread handle not inheritable.
						TRUE,            // inherite handle.
						0,                // No creation flags.
						NULL,             // Use parent's environment block.
						NULL,             // Use parent's starting directory.
						&si,              // Pointer to STARTUPINFO structure.
						&pi))            // Pointer to PROCESS_INFORMATION structure.			
	{
		fprintf(stderr, "Process Creation Failed\n");
		return -1;
	}

	sprintf(buffer, "%d", numVes);

	/* Haifa port now sends ships to the pipe(souze) */
	if (!WriteFile(StdInWrite, buffer, BUFFER_SIZE, &written, NULL))
		printf(stderr, "Error writing to pipe-father\n");

	//Read response from Eilat 
	if (ReadFile(StdOutRead, buffer, BUFFER_SIZE, &read, NULL)) {
		if (atoi(buffer) == 1) {
			printf("Sorry, you cant send %d ships to eilat port!", numVes);
			/* close all handles */
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			exit(0);
		}
	}
	else
		printf(stderr, "haifa: Error reading from pipe\n");

	//Initializa global data after confirmation from Eilat
	if (initGlobalData() == False)
	{
		printf("main::Unexpected Error in Global Semaphore Creation\n");
		return 1;
	}

	//Initialise vessels Threads
	for (int i = 0; i < numVes; i++)
	{
		vesselID[i] = i + 1;
		vessels[i] = CreateThread(NULL, 0, Vessel, &vesselID[i], 0, &ThreadId);
		if (vessels[i] == NULL)
		{
			printf("main::Unexpected Error in Vessel %d Creation\n", i);
			return 1;
		}
	}

	int i = 0;

	while (ReadFile(StdOutRead, buffer, BUFFER_SIZE, &read, NULL)) {

		if (strcmp(buffer, "") != 0) {
			printf("[%s] Vessel %d - done sailing @ Haifa Port \n", timePrinting(), atoi(buffer));
			Sleep(calcSleepTime());
				if (!ReleaseSemaphore(semVessel[(atoi(buffer) - 1)], 1, NULL)) {
				printf("vesselThatCameBack::Unexpected error semVessel.V()\n");
				}
			i++;
			if (i == numVes)
				break;
		}

	}

	WaitForMultipleObjects(numVes, vessels, TRUE, INFINITE);
	printf("[%s] Haifa Port: All Vessel Threads are done\n", timePrinting());
	
	/* close all pipes */
	CloseHandle(StdOutWrite);
	CloseHandle(StdInRead);
	CloseHandle(si.hStdError);

	WaitForSingleObject(pi.hProcess, INFINITE);
	WaitForSingleObject(pi.hThread, INFINITE);

	/* close all handles */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	//close all global Semaphore Handles
	cleanupGlobalData();
	printf("[%s] Haifa Port: Exiting....\n", timePrinting());
}

//thread function for vessels thread by unique ID
DWORD WINAPI Vessel(PVOID Param)
{
	int vesselId = *(int*)Param;

	printf("[%s] Vessel % d starts sailing  @ Haifa Port \n", timePrinting(), vesselId);
	Sleep(calcSleepTime());
	toSuez(vesselId);
	Sleep(calcSleepTime());

	return 0;
}

//generic function to randomise a Sleep time between 1 and MAX_SLEEP_TIME msec
int calcSleepTime()
{
	srand((unsigned int)time(NULL));
	return(rand() % MAX_SLEEP_TIME + MIN_SLEEP_TIME);
}

//function that send the vessels frome Haifa to suez
void toSuez(int id)
{
	WaitForSingleObject(mutex, INFINITE);

	printf("[%s] Vessel %d - entering Canal: Med. Sea ==> Red Sea  \n", timePrinting(), id);
	Sleep(calcSleepTime());
	int sizeBuf = sizeof(inttostr(id));
	if (!WriteFile(StdInWrite, inttostr(id), BUFFER_SIZE, &written, NULL))
		fprintf(stderr, "Error writing to pipe-eilat\n");

	if (!ReleaseMutex(mutex))
		printf("toSuez::Unexpected error mutex.V()\n");

	WaitForSingleObject(semVessel[id - 1], INFINITE);
}

//generic function to initial all mutex\semaphore and allocated memory to variable
int initGlobalData()
{
	int i;
	vessels = (HANDLE*)malloc(numVes * sizeof(HANDLE));
	vesselID = (int*)malloc(numVes * sizeof(int));
	semVessel = (HANDLE*)malloc(numVes * sizeof(HANDLE));
	if ((vessels || vesselID || semVessel) == NULL) {
		printf("memory error!!!");
		return -1;
	}

	mutex = CreateMutex(NULL, FALSE, NULL);
	if ((mutex) == NULL)
	{
		return False;
	}

	for (i = 0; i < numVes; i++)
	{
		semVessel[i] = CreateSemaphore(NULL, 0, 1, NULL);
		if (semVessel[i] == NULL)
		{
			return False;
		}
	}

	return True;
}

//Close all global semaphore handlers - after all vessels Threads finish.
void cleanupGlobalData()
{
	int i;

	CloseHandle(mutex);

	for (i = 0; i < numVes; i++)
		CloseHandle(semVessel[i]);
	free(vessels);
	free(vesselID);
	free(semVessel);

}

//generic function that change int to string
char* inttostr(int n) {
	char* result;
	if (n >= 0)
		result = malloc(floor(log10(n)) + 2);
	else
		result = malloc(floor(log10(n)) + 3);
	sprintf(result, "%d", n);
	return result;
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