import threading, time
class FakeSerial:
    """
    simulation of serial.Serial for testing when xbee is not available
    """
    def __init__(self, timeout):
        self.timeout = timeout #in seconds
        self.buffer = bytearray() #FIFO
        self.lock = threading.Lock()
        self.fileSim = False


    def read(self, size):
        enter = self.timeout
        while len(self.buffer) < size:
            #print('wait')
            if enter < 0:
                #print("simtimeout")
                return [] #TIMEOUT
            time.sleep(.1)
            enter-= .1

        with self.lock:
            value = self.buffer[-size:]
            self.buffer = self.buffer[:-size]
            #time.sleep(1)
            return value
    
    def WriteBufferData(self, data: bytearray):
        with self.lock:
            if(isinstance(data, str)):
                data = bytearray(bytes(data, encoding='ascii'))[::-1]
            data.extend(self.buffer)
            self.buffer = data
            #print(self.buffer)
    
    def _FileSim(self, fileSimContents): #[ (2, "awdw") , ...]
        if self.fileSim:
            print("cannot start another sim")
            return
        print("****BEGIN FILE SIMULATION****")
        self.fileSim = True

        for wait, data in fileSimContents:
            time.sleep(wait)
            self.WriteBufferData(data)
        time.sleep(self.timeout*5)
        print("****END FILE SIMULATION****")
        self.fileSim = False
    
    def BeginFileSim(self, fileName="sim_data.txt"):
        with open(fileName, 'r') as fread:
            lines = fread.readlines()
        formatted = []
        for line in lines :
            w, data = line.replace("\n", "").split(':')
            formatted.append((int(w), data))

        #formatted = [(int(w), data) for line in lines for w, data in line.split(":") ]
        
        #print(formatted)
        sim = threading.Thread(target=self._FileSim, args=(formatted,))
        sim.start()
