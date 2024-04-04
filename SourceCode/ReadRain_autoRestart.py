# ReadRain.py
# PythonScript which saves minutly PrecipationData from a connected Davs Rain Gauge to an sql lite database
# script is auto shutdown on minute 20min57s each hour - so has to be restarted at minute 21 via crontab - 3s of blind spot each hour.
# Robert Krüger - TUD

import RPi.GPIO as GPIO
import sqlite3
from datetime import *
import time
import os

# initalize minute count
count = 0

GPIO.setmode(GPIO.BOARD)
# Sensor Pin noch zu setzen
GPIO.setup(11, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)

def itsraining(channel):
    global count
    count += 1

GPIO.add_event_detect(11, GPIO.FALLING, callback=itsraining, bouncetime=2000)
start = datetime.utcnow().second
if start < 2:
    startup = 0
else:
    startup = 1

try:
    while True:
        try:
            now = datetime.utcnow()
            if now.minute == 20 and now.second > 50:
                now = now.replace(second = 0, microsecond = 0) + timedelta(minutes = 1)
                conn = sqlite3.connect('/home/pi/Documents/database/Rain.db')
                c = conn.cursor()
                print("INSERT INTO data VALUES (" +     str(count) + ', "' + str(now) + '", 0)')
                if startup == 0:
                    c.execute("INSERT INTO data VALUES (" + str(count) + ', "' + str(now) + '", 0)')
                conn.commit()

                os.system('sudo killall python')
            
            now = now.replace(second = 0, microsecond = 0)
            sleeptime = ((now + timedelta(minutes = 1)) - datetime.utcnow()).total_seconds()
            time.sleep(sleeptime)

            #read time and count for last minute -> save to db
            now = datetime.utcnow().replace(second = 0, microsecond = 0)
                        
            conn = sqlite3.connect('/home/pi/Documents/database/Rain.db')
            c = conn.cursor()
            
            if startup == 0:
                print("INSERT INTO data VALUES (" +     str(count) + ', "' + str(now) + '", 0)')
                c.execute("INSERT INTO data VALUES (" + str(count) + ', "' + str(now) + '", 0)')
            conn.commit()
            #reset counter
            startup = 0
            count = 0            

            #sleep for 57 seconds
            sleeptime = ((now + timedelta(seconds = 57)) - datetime.utcnow()).total_seconds()
            time.sleep(sleeptime)
        
        except KeyboardInterrupt:
            GPIO.cleanup()
        
        except Exception as e:
            print(e)
            time.sleep(1)
            continue

except KeyboardInterrupt:
            GPIO.cleanup()
GPIO.cleanup()
