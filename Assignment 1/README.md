SYSC 4001 Assignment 1
======================
(last updated README.md on October 08, 2015)

Emulation of Internet of Things (IoT) using Software Process.

Building
========
make all

Running
=======
On seperate terminals:

1. Run Controller
bin/controller NAME

2. Run Cloud
bin/cloud NAME

3. Run as many Devices as needed:
bin/sensor NAME THRESHOLD MAXIMUM-SIMULATED-VALUE
or
bin/actuator NAME

Querying Devices
================
Querying devices can be done on the Cloud. Write the following on
stdin of the Cloud process.

Get PID
Put PID "MESSAGE"

Get will query Sensor with PID.
Put will send MESSAGE to Actuator with PID.

Ending Execution
================
Ending execution should be done by sending SIGINT (ctrl-c) to the
Controller process. The Controller process will shut down all related
processes and shut down gracefully.


