#!/usr/bin/env python3

import matplotlib.pyplot as plt
import pandas as pd
import sqlite3
import sys
from os import path
import pathlib
from datetime import datetime, date, time, timedelta
import matplotlib.dates as mdates

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

sql_connection = sqlite3.connect(sys.argv[1], detect_types=sqlite3.PARSE_DECLTYPES)
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

        # roughly 1 per second, but with millisecond precision unfortunately
        data = pd.read_sql_query("SELECT time, kelvin, lux from %s WHERE time BETWEEN ? AND ?" % TCS, sql_connection, params = sql_params)
        data['lux'] = data['lux'].abs() # sometimes there are negative values there...?

        # https://stackoverflow.com/questions/40085300/how-to-smooth-lines-in-a-figure-in-python
        # from scipy.interpolate import make_interp_spline, BSpline
        # data = data.set_index('time').asfreq('1min', method='pad').reset_index()
        # data = data.set_index('time').resample('S').mean().interpolate().asfreq('30min', method='pad').reset_index()
        # Sri says: if you want to get rid of the details, maybe try throwing stuff out first, then interpolate with spline or cubic afterwards?
        # print(data)
        # sys.exit()

        if DEBUG:
            with pd.option_context('display.max_rows', None, 'display.max_columns', None):
                print(data)
        else:
          print("Plotting %s" % date_str)

        fig, ax = plt.subplots(figsize=(12,6))
        plot_title = "Light temperature (K) throughout %s" % date_str
        data.plot(kind='area', title=plot_title, x='time', y='lux', color='#FFF3D8', ax=ax)
        data.plot(kind='line', title=plot_title, x='time', y='kelvin', color='#FFB515', ax=ax)
        ax.set_xlabel('Hour')
        ax.set_xlim(datetime.combine(first_day + timedelta(days = offset), time(3, 0, 0)), datetime.combine(first_day + timedelta(days = offset), time(23, 0, 0)))
        ax.set_ylabel('Temperature (K)')
        ax.set_ylim(1500, 10500)
        hours = mdates.HourLocator()
        fmt = mdates.DateFormatter('%H')
        ax.xaxis.set_major_locator(hours) 
        ax.xaxis.set_major_formatter(fmt)

        pathlib.Path("plots").mkdir(exist_ok=True)
        plt.tight_layout()
        plt.savefig(filename)

sql_connection.close()
