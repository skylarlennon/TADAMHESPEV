% Skylar Lennon
% EECS 373 
% FA23
% This script is designed to help size the resistors for a voltage divider
% which will be used to step down the battery voltage to be input to the
% ADC of the STM32L4R4 and maximize the range used of the ADC

clc;clear;

syms r1 r2

Vmin = 46;                  %Volts
Vmax = 54;                  %Volts
V_array = linspace(Vmin,Vmax,1000);
ADC_Bits = 12;
Vref_ADC = 3.3;             %Volts
maxPowerVoltMeter = 0.25;   %Watts

%Create a graph for power consumed as a function of battery voltage
%and the total resistance of the voltage divider
peqn = maxPowerVoltMeter == Vmax^2/(r1 + r2);
vd_eqn = Vmax * r2 / (r1 + r2) == 3.3;

a = solve([peqn vd_eqn], [r1 r2]);

r1Val = a.r1;
r2Val = a.r2;

R_array = linspace(0,r1Val+r2Val+2000,1000);

[V_AR,R_AR] = meshgrid(V_array,R_array);

VoltageRange = (Vmax*r1Val/(r1Val + r2Val)) - (Vmin*r1Val/(r1Val + r2Val));
numADCVals = vpa((2^12/Vref_ADC) * VoltageRange, 4);
adcResolution = vpa(VoltageRange / numADCVals, 4); %volts/adcVal





