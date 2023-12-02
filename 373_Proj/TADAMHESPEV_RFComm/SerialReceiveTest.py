import serial, time
from datetime import datetime, timedelta
import argparse, struct
from SerialReader import SerialReader
from TADAMHESPEVPacket import TADAMHESPEVPacket, LoadTADAMHESPEVLogFile
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


    


def MessageRecieved(message: bytearray):
    print("MESSAGE RECIEVED: {}".format(message))
    packet = TADAMHESPEVPacket(message)
    print("in other words", str(packet), '\n\n')

reader = SerialReader(args.port)
reader.AddRecieveCallback(MessageRecieved)
reader.StartRead()

while True:
    time.sleep(.5)
