import struct
from datetime import datetime
class TADAMHESPEVPacket:
    #buf[0] accel, buf[1] temp, buf[2] speed, buf[3] voltage, buf[4] current
    def __init__(self, bytepacket: bytearray, timeRecieved = None):
        self.packet = self.ByteArrayToFloatArray(bytepacket)
        self.accel = self.packet[0]
        self.temp = self.packet[1]
        self.speed = self.packet[2]
        self.voltage = self.packet[3]
        self.current = self.packet[4]

        self.time = timeRecieved if timeRecieved else datetime.now()
    
    @staticmethod
    def ByteArrayToFloatArray( bytearr):
        floatArr = []
        i = 0
        while i < len(bytearr): #https://stackoverflow.com/questions/1592158/convert-hex-to-float
            floatArr.append(struct.unpack('!f', bytearr[i:i+4][::-1])[0]) #reverse for endianness
            i = i+4
        return floatArr
    
    def __str__(self):
        return ", ".join(map(str, self.packet))

class TADAMHESPEVDatabase:
    def __init__(self):
        self.packets = []