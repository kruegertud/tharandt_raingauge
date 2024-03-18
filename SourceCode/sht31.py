# Python script which reads Data from an SHT31 Temperature / Humidity Sensor and writes the Data to a SQLlite Database
# in this version - Data from a Pyranometer is read as well through a ADC
# Robert Krüger TU Dresden

import TeelSys_SHT.SHT31 as SHT31
import time
from datetime import datetime
import sqlite3
import Adafruit_ADS1x15

sht31 = SHT31.SHT31()
adc = Adafruit_ADS1x15.ADS1115()
GAIN = 1

conn = sqlite3.connect('/home/pi/Documents/database/sht31.db')
c = conn.cursor()

temp = sht31.read_temperature()
hum = sht31.read_humidity()
now = datetime.utcnow().replace(second = 0, microsecond = 0)

values = []

for i in range(0,100):
    values.append(adc.read_adc(1, gain=GAIN, data_rate=16))
    time.sleep(0.07)
    i += 1

s = sorted(values)
inner = []
for i in range(int(0.3*len(s)),int(0.7*len(s))):
    inner.append(s[i])

med = inner[int((len(inner)/2))]
flux = int(float(med) / 32767) * 4096 * 0.8

print(flux, temp, hum)

try:
    c.execute("INSERT INTO data VALUES ('T', " + str(format(temp, '.1f')) + ', "' + str(now) + '", 0)')
    c.execute("INSERT INTO data VALUES ('H', " + str(format(hum, '.1f')) + ', "' + str(now) + '", 0)')
    c.execute("INSERT INTO data VALUES ('F', " + str(format(flux, '.1f')) + ', "' + str(now) + '", 0)')
    conn.commit()

except Exception as e:
    print(e)
    time.sleep(1)
    continue