#https://github.com/CoreyMSchafer/code_snippets/blob/master/Python/Matplotlib/09-LiveData/finished_code.py
#https://www.youtube.com/watch?v=Ercd-Ip5PfQ 
import random
from itertools import count
#import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import threading, time
from datetime import datetime, timedelta
import argparse
from TADAMHESPEVPacket import TADAMHESPEVPacket

parser = argparse.ArgumentParser(description="Serial Test")
parser.add_argument("-s", "--seconds", default=10)
parser.add_argument("--record-only", action="store_true")

args = parser.parse_args()
print(args.__dict__)
seconds = int(args.seconds)
#exit()

plt.style.use('classic')

index = count()

TIMES  = [i for i in range(-seconds, 0)]#[]
print(TIMES)
DATABASE = {
    "acceleration": [],
    "current": [],
    "velocity": [],
    "voltage": []
}

#fig, ((pVolt, pCurr), (pAcc, pVel)) = plt.subplots(nrows=2, ncols=2, sharex=True)
fig, (pVolt, pCurr, pAcc, pVel) = plt.subplots(nrows=4, ncols=1, sharex=True)
allPlots = [pVolt, pCurr, pAcc, pVel]



def UpdatePlots(i):
    xTime = TIMES[-len(DATABASE['acceleration']):]
    yAcc = DATABASE['acceleration'][-seconds:]
    yCurr = DATABASE['current'][-seconds:]
    yVolt = DATABASE['voltage'][-seconds:]
    yVelocity = DATABASE['velocity'][-seconds:]

    #clear all plots
    for p in allPlots:
        p.cla()

    pVolt.plot(xTime, yVolt, label='Voltage')
    pVolt.set_ylabel("Voltage (v)")
    

    pAcc.plot(xTime, yAcc, label='Acceleration', color = '#ed1a1a')
    pAcc.set_ylabel("Acceleration (m/s^2)")

    pCurr.plot(xTime, yCurr, label='Current Draw', color = '#1d9c00')
    pCurr.set_ylabel("Current Draw (amp)")

    pVel.plot(xTime, yVelocity, label='Velocity', color = '#c217b6')
    pVel.set_ylabel("Velocity (m/s)")
    pVel.set_xlabel("Time (s)")
    pVel.set_xlim(-seconds, -1)
    #plt.legend(loc='upper left')
    
"""
def DataUpdateThread(): #simulate incoming data
    while True:
        try:
            #TIMES.append(next(index))#(str(datetime.utcnow()))
            for val in DATABASE.values():
                val.append(random.randint(2, 20))
            time.sleep(.5)
        except Exception as ex:
            print('exception in DataUpdateThread: ', ex)
            exit(1)

updateThread = threading.Thread(target=DataUpdateThread,args=(), daemon=True)
updateThread.start()
"""
def DataRecieved(message: bytearray):
    print("MESSAGE RECIEVED: {}".format(message))
    packet = TADAMHESPEVPacket(message)
    print("in other words", str(packet), '\n\n')

reader = SerialReader(args.port)
reader.AddRecieveCallback(MessageRecieved)
reader.StartRead()

ani = FuncAnimation(plt.gcf(), UpdatePlots, interval=1000, cache_frame_data=False)
plt.tight_layout()

if not args.record_only:
    print("Begining plot")
    plt.show()
else:
    print("record-only: Not showing graphs")
    while 1:
        time.sleep(.1)

print('exit')