import serial, time
from datetime import datetime, timedelta
import argparse
from SerialReader import SerialReader
parser = argparse.ArgumentParser(description="Serial Test")
parser.add_argument("-p", "--port")

args = parser.parse_args()
print("Starting read from", args.port)

"""
ser = serial.Serial(args.port, 9600)

lastReadTime = datetime.now()
message = bytearray()

while True:
    s = ser.read(1)
    newReadTime = datetime.now()
    if newReadTime - lastReadTime > timedelta(milliseconds=200) and len(message)>0: #start new message
        print(str(message))
        message = bytearray()
    lastReadTime = newReadTime
    message.extend(s)
"""

def MessageRecieved(message):
    print("MESSAGE RECIEVED: {}".format(message))

reader = SerialReader(args.port)
reader.AddRecieveCallback(MessageRecieved)
reader.StartRead()

while True:
    time.sleep(.5)
