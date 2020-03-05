#!/usr/bin/env python3

import matplotlib.pyplot as plt
import pandas as pd
import sqlite3
from os import path
import pathlib
from datetime import datetime, date, time, timedelta

# TODO: why not working?
# import matplotlib.dates as mdates
# hours = mdates.HourLocator()
# fmt = mdates.DateFormatter('%H')
# ax.xaxis.set_major_locator(hours) 
# ax.xaxis.set_major_formatter(fmt)

TCS = "tcs34725"
APDS = "apds9960"
DEBUG = False

def get_first_date():
    sql.execute("SELECT time FROM %s ORDER BY time ASC LIMIT 1" % TCS)
    rows = sql.fetchall()
    return rows[0][0].date()

def get_last_date():
    sql.execute("SELECT time FROM %s ORDER BY time DESC LIMIT 1" % TCS)
    rows = sql.fetchall()
    return rows[0][0].date()

sql_connection = sqlite3.connect("rgbsensortest_data.sqlite3", detect_types=sqlite3.PARSE_DECLTYPES)
sql = sql_connection.cursor()

first_day = get_first_date()
last_day = get_last_date()
n_days = (last_day - first_day).days + 1

for offset in range(n_days):
    first_min = datetime.combine(first_day + timedelta(days = offset), time(0, 0, 1))
    last_min = datetime.combine(first_day + timedelta(days = offset), time(23, 59, 59))
    date_str = first_min.strftime('%Y-%m-%d')
    date_today_str = date.today().strftime('%Y-%m-%d')
    filename = "plots/%s.png" % date_str

    if not path.isfile(filename) or date_str == date_today_str:
        sql_params = [first_min, last_min]
        data = pd.read_sql_query("SELECT time, kelvin from %s WHERE time BETWEEN ? AND ?" % TCS, sql_connection, params = sql_params)

        # TODO: works fine but there are holes in the data - why?!
        # data = data.set_index('time').groupby(pd.Grouper(freq='T')).mean().reset_index()

        if DEBUG:
            with pd.option_context('display.max_rows', None, 'display.max_columns', None):
                print(data)
        else:
          print("Plotting %s" % date_str)

        fig, ax = plt.subplots(figsize=(12,6))
        plot_title = "Light temperature (K) throughout %s" % date_str
        data.plot(kind='line', title=plot_title, x='time', y='kelvin', legend=False, ax=ax)
        ax.set_xlabel('Hour')
        ax.set_xlim(datetime.combine(first_day + timedelta(days = offset), time(3, 0, 0)), datetime.combine(first_day + timedelta(days = offset), time(23, 0, 0)))
        ax.set_ylabel('Temperature (K)')
        ax.set_ylim(1500, 10000)

        pathlib.Path("plots").mkdir(exist_ok=True)
        plt.savefig(filename)

sql_connection.close()
