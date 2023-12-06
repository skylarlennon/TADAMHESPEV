import struct, pandas #pip install pandas
from datetime import datetime
class TADAMHESPEVPacket:
    #buf[0] accel, buf[1] temp, buf[2] speed, buf[3] voltage, buf[4] current
    def __init__(self, bytepacket: bytearray, timeRecieved = None):
        self.bytepacket = bytepacket
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
    
    
    def CSVString(self):
        strArr = []
        i = 0
        while i < len(self.bytepacket): 
            strArr.append( self.bytepacket[i:i+4][::-1].hex() ) #reverse for endianness
            i = i+4
        return ",".join(strArr)
    
    def __str__(self):
        return ", ".join(map(str, self.packet))


def HexStrToFloat(hexstr): #https://stackoverflow.com/questions/1592158/convert-hex-to-float
    return struct.unpack('!f', bytes.fromhex(hexstr))[0]

COLUMNS = ['Acceleration', 'Temperature', 'Speed', 'Voltage', 'Current']
def LoadTADAMHESPEVLogFile(fileName = "TADAMHESPEVLog.csv"):
    db = pandas.read_csv(fileName, converters={i: str for i in range(10000)})#, usecols=['Acceleration', 'Temperature', 'Speed', 'Voltage', 'Current'])
    #print(db)
    """
    print(db)
    accVals = db["Speed"]
    print(accVals)
    print(list(accVals))
    print(list(map(HexStrToFloat, list(accVals))))
    with open(fileName, 'r') as csvfile:
        csvreader = csv.reader(csvfile)
        print(csvreader.__dict__)
    """
    return {
        colname: list(map(HexStrToFloat, list(db[colname]))) for colname in COLUMNS
    }

def ReplayTADAMHESPEVLogFile(fileName = "TADAMHESPEVLog.csv" ): #generator
    index = 0
    db = pandas.read_csv(fileName, converters={i: str for i in range(10000)})
    while 1:
        for i in range(2, len(db)):
            yield {
            colname: list(map(HexStrToFloat, list(db[colname])[:i])) for colname in COLUMNS
        }
    
