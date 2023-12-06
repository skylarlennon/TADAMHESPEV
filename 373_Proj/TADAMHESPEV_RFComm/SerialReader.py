import serial, time, threading
from datetime import datetime, timedelta
from FakeSerial import FakeSerial
class SerialReader:
    SIM_PORT = "SIM"

    @classmethod
    def Simulation(cls):
        return SerialReader(SerialReader.SIM_PORT)

    def __init__(self, port, baud = 9600, messageTimeDelta = 200):
        """
        reader object to simplify serial reading process
        messageTimeDelta is time in ms between messages to accept
        """
        self._port = port
        self._baud = baud
        self.messageTimeDelta = messageTimeDelta
        self._listening = False
        self._recieveCallbacks = []

        self.lastMessage = None
        self.lastReadTime = None

        self._SerialInit()

        self._readThread = threading.Thread(target=self._ReadThread,args=(), daemon=True)
        self._readThread.start()
    
    def _SerialInit(self):
        if self._port == SerialReader.SIM_PORT:
            self.ser = FakeSerial(timeout=self.messageTimeDelta/1000)
        else:
            self.ser = serial.Serial(self._port, self._baud, timeout=self.messageTimeDelta/1000)
        
    def AddRecieveCallback(self, callback):
        self._recieveCallbacks.append(callback)

    def _OnMessageRecieved(self, message):
        self.lastMessage = message
        for cb in self._recieveCallbacks:
            try:
                cb(message)
            except Exception as ex:
                print("Exception in callback {}: {}".format(cb, ex))

    def _ReadThread(self):
        print('start read thread')
        while 1:
            try:
                self.lastReadTime = datetime.now()
                message = bytearray()
                while self._listening:
                    s = self.ser.read(1)
                    if len(s) == 0: #timeout 
                        if len(message)>0:
                            self._OnMessageRecieved(message)
                            message = bytearray()
                        continue
                    
                    newReadTime = datetime.now()
                    if newReadTime - self.lastReadTime > timedelta(milliseconds=self.messageTimeDelta) and len(message)>0: #start new message
                        self._OnMessageRecieved(message)
                        message = bytearray()
                    self.lastReadTime = newReadTime
                    message.extend(s)
            except Exception as ex:
                print('here')
                raise ex
                print("EXCEPTION WHEN READING DATA: {}".format(ex))

    def StartRead(self):
        self._listening = True
    
    def StopRead(self):
        self._listening = False


