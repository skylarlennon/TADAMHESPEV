% Skylar Lennon
% EECS 373 
% FA23
% This script is designed to help size the resistors for a voltage divider
% which will be used to step down the battery voltage to be input to the
% ADC of the STM32L4R4 and maximize the range used of the ADC

clc;clear;

syms r1 r2 rtot vb

Vmin = 40; %    Volts
Vmax = 54; %    Volts

%Create a graph for power consumed as a function of battery voltage
%and the total resistance of the voltage divider
P = vb^2/(rtot);




