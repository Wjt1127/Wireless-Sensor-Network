## Compile
* `make`

## Execute
* `mpirun -np P prog X Y T I`

Where ***P*** is number of processors, 
and ***X*** is number of rows of the grid and ***Y*** is the number of columns, and ***X * Y + 1 = P***. 

***T*** is the time of each iteration.

***I*** is the number of iterations to run for.

## Test Example

* run example
```bash
make
mpiexec -n 21 ./sim_proc 4 5 50 10
```

* base station log
```
------------------------------------------------------------------------------------------------------------
Iteration : 4
Logged time : 					Thu Oct  5 01:14:02 2023
Alert reported time : 			Thu Oct  5 01:14:02 2023
Number of adjacent node : 4
Availability to be considered full : 1

Reporting Node 	 Coord 		 Port Value 	 Available Port 	 IPv4
13				 (2,3)		 5				 0					 192.168.195.189

Adjacent Nodes 	 Coord 		 Port Value 	 Available Port 	 IPv4
8				 (1,3)		 5				 1					 192.168.195.189
18				 (3,3)		 5				 1					 192.168.195.189
12				 (2,2)		 5				 1					 192.168.195.189
14				 (2,4)		 5				 1					 192.168.195.189

Nearby Nodes 	 Coord 	
3				 (0,3)
7				 (1,2)
9				 (1,4)
11				 (2,1)
17				 (3,2)
19				 (3,4)

Available station nearby (no report received in last 3 iteration) : 3,7,9,11,17,19
Communication Time (seconds) : 0.000541
Total Messages send between reporting node and base station: 2
------------------------------------------------------------------------------------------------------------
```