#!/usr/bin/env python3

import os.path
import sys
import paho.mqtt.client as mqtt # https://pypi.org/project/paho-mqtt
import numpy as np
import colour # https://github.com/colour-science/colour
import sqlite3 # lightweight database, https://sqlite.org
import datetime

sql_connection = sqlite3.connect("rgbsensortest_data.sqlite3")
sql_connection.set_trace_callback(print)
sql = sql_connection.cursor()

def extract_values(str):
    parts = str.split(";")
    sensor1 = map(int, parts[0].split(","))
    sensor2 = map(int, parts[1].split(","))
    lux1, tmp1 = sensor1
    lux2, r, g, b = sensor2
    tmp2 = 0
    if r > 0 or g > 0 or b > 0:
        rgb = np.array([r, g, b])
        xyz = colour.sRGB_to_XYZ(rgb / 255)
        xy = colour.XYZ_to_xy(xyz)
        tmp2 = int(colour.xy_to_CCT(xy, "hernandez1999"))
    if tmp2 < 0:
        tmp2 = 0
    return [lux1, tmp1, lux2, tmp2]

def store_data(values):
    now = datetime.datetime.now()
    lux1, tmp1, lux2, tmp2 = values
    # print("TCS34725 data: ", [now, lux1, tmp1])
    # print("APDS9960 data: ", [now, lux2, tmp2])
    with sql_connection:
        sql.execute("INSERT INTO tcs34725 VALUES (?, ?, ?)", (now, lux1, tmp1))
        sql.execute("INSERT INTO apds9960 VALUES (?, ?, ?)", (now, lux2, tmp2))

def on_connect(client, userdata, flags, result_code):
    client.subscribe("RGBSensorTest/values")

def on_message(client, userdata, message):
    msg = str(message.payload.decode("utf-8"))
    # print("message received: ", msg)
    values = extract_values(msg)
    # print("values extracted: ", values)
    store_data(values)

if os.path.getsize("rgbsensortest_data.sqlite3") == 0:
    with sql_connection:
        sql.execute("CREATE TABLE IF NOT EXISTS tcs34725 (time timestamp, lux integer, kelvin integer)")
        sql.execute("CREATE TABLE IF NOT EXISTS apds9960 (time timestamp, lux integer, kelvin integer)")
    print("Created new database in rgbsensortest_data.sqlite3.")

mqtt_client = mqtt.Client(client_id="rgbsensor_reader")
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(sys.argv[1])

try:
    print("Getting data from MQTT server and storing in database.")
    print("Press Ctrl-C to stop.", end = '', flush = True)
    mqtt_client.loop_forever()
except KeyboardInterrupt:
    pass

res1 = sql.execute("SELECT COUNT(time) FROM tcs34725")
val1 = res1.fetchone()[0]
res2 = sql.execute("SELECT COUNT(time) FROM apds9960")
val2 = res2.fetchone()[0]
records = val1 + val2
print("\nStopped with %i records in database (%i for TCS34725 and %i for APDS9960)." % (records, val1, val2))
sql_connection.close()
