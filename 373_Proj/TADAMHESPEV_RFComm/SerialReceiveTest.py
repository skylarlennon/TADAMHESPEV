import serial, time
from datetime import datetime, timedelta
ser = serial.Serial('COM4', 9600)

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

