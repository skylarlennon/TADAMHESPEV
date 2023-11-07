#https://github.com/CoreyMSchafer/code_snippets/blob/master/Python/Matplotlib/09-LiveData/finished_code.py
#https://www.youtube.com/watch?v=Ercd-Ip5PfQ 
import random
from itertools import count
#import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import threading, time
from datetime import datetime, timedelta

plt.style.use('fivethirtyeight')


index = count()

DATABASE = {
    "time": [],
    "acceleration": [],
    "power": []
}

def animate(i):
    x = DATABASE['time']
    yAcc = DATABASE['acceleration']
    yPow = DATABASE['power']

    plt.cla()

    plt.plot(x, yAcc, label='Acceleration')
    plt.plot(x, yPow, label='Power')

    plt.legend(loc='upper left')
    plt.tight_layout()

def DataUpdateThread():
    while True:
        try:
            DATABASE['time'].append(next(index))#(str(datetime.utcnow()))
            DATABASE['acceleration'].append(random.randint(2,12))
            DATABASE['power'].append(random.randint(1,6))
            time.sleep(.5)
        except Exception as ex:
            print('exception in DataUpdateThread: ', ex)
            exit(1)

updateThread = threading.Thread(target=DataUpdateThread,args=(), daemon=True)
updateThread.start()

ani = FuncAnimation(plt.gcf(), animate, interval=1000, cache_frame_data=False)

plt.tight_layout()
plt.show()
print('exit')