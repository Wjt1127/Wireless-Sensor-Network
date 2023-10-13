## Compile
* `make`

## Execute
* `mpirun -np P program X Y T I PN`

Where ***P*** is number of processors, 
and ***X*** is number of rows of the grid and ***Y*** is the number of columns, and ***X * Y + 1 = P***. 

***T*** is the time of each iteration.

***I*** is the number of iterations to run for.

***PN*** is the number of each evnode

## Test Example

* run example
```bash
make
mpiexec -n 21 ./sim_proc 4 5 50 10 2
```

* base station log
```
Iteration : 1
Logged time : 					Fri Oct 13 11:37:24 2023
Alert reported time : 			Fri Oct 13 11:37:24 2023
Number of adjacent node : 3
Availability to be considered full : 1

Reporting Node 	 Coord 		 Port Value 	 Available Port 	 IPv4
5				 (1,0)		 5				 0					 192.168.195.189

Adjacent Nodes 	 Coord 		 Port Value 	 Available Port 	 IPv4
0				 (0,0)		 5				 0					 192.168.195.189
10				 (2,0)		 5				 0					 192.168.195.189
6				 (1,1)		 5				 0					 192.168.195.189

Nearby Nodes 	 Coord 	
1				 (0,1)
7				 (1,2)
11				 (2,1)
15				 (3,0)

Available station nearby: 1,7,11,15
Total Messages send between reporting node and base station: 2

```