# Multithreading-in-Win32Api
2 Processes that communicate with each other. Haifa port is activated and send ships to Eilat port for unpacking and then return them to Haifa
At first Haifa Port get a number in the command line of ships and check if the number is between 2-51 else return Error and kill the Process.
After the ships created in Haifa (each ship is a thread with unique Id), they start to sail to Eilat port, when they arrived at Eilat Port The Eilat Process check if the ship's number is not Prime else send Error and kill the Process. 
In Eilat Port, the Ships stop at a Barrier before unpacking, and only if the barrier capacity fits the Crane number that supposes to unpacking the ships they get out of the Barrier to the Unloading Quay for unpacking the cargo. (The Barrier works like a Queue in FIFO).
Each Ship (Thread) that finishes unpacking the cargo sail back to Haifa Port.
Only if all the Ships return to Haifa Port the Main Finish.

There are more restrictions like that the Crane number is between 1 to the number of ships etc.
