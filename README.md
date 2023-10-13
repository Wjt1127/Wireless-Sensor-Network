## Compile
* `make`

## Execute
* `mpirun -np N program X Y T IN PN`

Where ***N*** is the number of MPI processors, 
***X***, ***Y*** is the number of rows and columes of the grid and they have relation of ***N = X * Y + 1***. 

***T*** is the time(s) of each iteration.

***IN*** is the number of iterations to run.

***PN*** is the ports number of each evnode

## Test Example

* run example
```bash
make
mpiexec -n 21 ./sim_proc 4 5 50 10 2
```

* ev node log
```
Fri Oct 13 14:01:38 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 2
Fri Oct 13 14:01:48 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 1
Fri Oct 13 14:01:48 2023, PROMPT_INFO: EVNode (1, 0) starts to prompt
Fri Oct 13 14:01:48 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (2, 0)'s availability is 2
Fri Oct 13 14:01:48 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (1, 1)'s availability is 2
Fri Oct 13 14:01:48 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (0, 0)'s availability is 2
Fri Oct 13 14:01:58 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 1
Fri Oct 13 14:01:58 2023, PROMPT_INFO: EVNode (1, 0) starts to prompt
Fri Oct 13 14:01:58 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (1, 1)'s availability is 2
Fri Oct 13 14:01:58 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (0, 0)'s availability is 2
Fri Oct 13 14:01:58 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (2, 0)'s availability is 0
Fri Oct 13 14:02:08 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 1
Fri Oct 13 14:02:08 2023, PROMPT_INFO: EVNode (1, 0) starts to prompt
Fri Oct 13 14:02:08 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (0, 0)'s availability is 0
Fri Oct 13 14:02:08 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (1, 1)'s availability is 1
Fri Oct 13 14:02:08 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (2, 0)'s availability is 1
Fri Oct 13 14:02:18 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 2
Fri Oct 13 14:02:28 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 2
Fri Oct 13 14:02:38 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 1
Fri Oct 13 14:02:38 2023, PROMPT_INFO: EVNode (1, 0) starts to prompt
Fri Oct 13 14:02:38 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (0, 0)'s availability is 0
Fri Oct 13 14:02:38 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (1, 1)'s availability is 1
Fri Oct 13 14:02:38 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (2, 0)'s availability is 1
Fri Oct 13 14:02:48 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 1
Fri Oct 13 14:02:48 2023, PROMPT_INFO: EVNode (1, 0) starts to prompt
Fri Oct 13 14:02:48 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (2, 0)'s availability is 1
Fri Oct 13 14:02:48 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (1, 1)'s availability is 1
Fri Oct 13 14:02:48 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (0, 0)'s availability is 1
Fri Oct 13 14:02:58 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 2
Fri Oct 13 14:03:08 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 0
Fri Oct 13 14:03:08 2023, PROMPT_INFO: EVNode (1, 0) starts to prompt
Fri Oct 13 14:03:08 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (0, 0)'s availability is 1
Fri Oct 13 14:03:08 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (1, 1)'s availability is 2
Fri Oct 13 14:03:08 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (2, 0)'s availability is 0
Fri Oct 13 14:03:18 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 1
Fri Oct 13 14:03:18 2023, PROMPT_INFO: EVNode (1, 0) starts to prompt
Fri Oct 13 14:03:18 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (1, 1)'s availability is 0
Fri Oct 13 14:03:18 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (0, 0)'s availability is 1
Fri Oct 13 14:03:18 2023, NEIGHBOR_AVAIL_INFO: EVNode (1, 0)'s neighbor (2, 0)'s availability is 1
Fri Oct 13 14:03:28 2023, AVAILABILITY_INFO: EVNode (1, 0)'s availability is 1
Fri Oct 13 14:03:28 2023, PROMPT_INFO: EVNode (1, 0) starts to prompt
```

* base station log
```
Iteration : 3
Logging time : 				Fri Oct 13 14:04:08 2023
Alert reporting time : 		Fri Oct 13 14:04:08 2023
Number of adjacent node : 2
Availability to be considered full : 1

Reporting Node 	 Coord 		 Port Value 	 Available Port 	 IPv4
19				 (3,4)		 5				 1					 192.168.195.189

Adjacent Nodes 	 Coord 		 Port Value 	 Available Port 	 IPv4
14				 (2,4)		 5				 0					 192.168.195.189
18				 (3,3)		 5				 0					 192.168.195.189

Nearby Nodes 	 Coord 	
9				 (1,4)
13				 (2,3)
17				 (3,2)

Available station nearby: 9,13,17
Total Messages send between reporting node and base station: 2
```