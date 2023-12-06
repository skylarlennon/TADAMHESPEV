import serial, time
from datetime import datetime, timedelta
import argparse, struct, os
from SerialReader import SerialReader
from TADAMHESPEVPacket import TADAMHESPEVPacket
parser = argparse.ArgumentParser(description="CSV Logger")
parser.add_argument("-p", "--port", required=True)
parser.add_argument("-f", "--filename", required = False)
args = parser.parse_args()
print("Starting read from", args.port)


logFileName = f"RFTADLOG.{str(datetime.now()).replace(' ', '_').replace(':', '--')}.csv" if not args.filename else args.filename
print("Logging to", logFileName)

#initialize file
if not os.path.exists(logFileName):
    print("Initializing", logFileName)
    with open(logFileName, 'w') as fwrite:
        fwrite.write("Acceleration,Temperature,Speed,Voltage,Current\n")


def MessageRecieved(message: bytearray):
    print("MESSAGE RECIEVED: {}".format(message))
    packet = TADAMHESPEVPacket(message)
    print("in other words", str(packet))
    print("in csv words", packet.CSVString())
    with open(logFileName, 'a') as fappend:
        fappend.write(packet.CSVString()+'\n')
    print("write success")

reader = SerialReader(args.port)
reader.AddRecieveCallback(MessageRecieved)
reader.StartRead()

while True:
    time.sleep(.5)
