# Mongoose OS Alarm Management Library

## Overview

A library which assists in the implementation of alarming in a Mongoose OS program.
Alarming in this instance relates to alarms that you would typically find in a industial PLC control system, where a number of analog and digital variables are monitored to figure out if a plant is running correctly. 
For Example:
  A pump sump system with an analog liquid level sensor will typically have a high high level(HHL), high level (HL), low level (LL) and low low level (LLL). When the level rises above a high level setpoint or below a low level setpoint the a boolean flag triggers and some rectifying routine is run (maybe a alert is sent to a person). 
  
 The logic above would be trival to implement in Mongoose OS, however, once the system gets even slightly more complex writing succinct reuseable code gets more difficult. This is where this library comes in! 
 
 A work in progress, full guide coming soon.
