import serial, time
from datetime import datetime, timedelta
import argparse
from SerialReader import SerialReader
parser = argparse.ArgumentParser(description="Serial SIM Test")
parser.add_argument("-p", "--port")

args = parser.parse_args()
#print("Starting read from", args.port)



def MessageRecieved(message):
    print("{} MESSAGE RECIEVED: {}".format(datetime.now(),message))

reader = SerialReader.Simulation()
serial = reader.ser
#serial.WriteBufferData("initial data")
reader.AddRecieveCallback(MessageRecieved)
reader.StartRead()
serial.BeginFileSim()
elapsed = 0
while True:
    time.sleep(.1)
    #elapsed += .1
    #print(elapsed)
    continue
    newdata = input("new data: ")
    serial.WriteBufferData(newdata)
    time.sleep(.5)
