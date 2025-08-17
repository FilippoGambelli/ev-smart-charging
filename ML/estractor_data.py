import pandas as pd
import pytz
from datetime import datetime

# SOLAR DATA

italy_tz = pytz.timezone('Europe/Rome')

df = pd.read_csv('dataset/test_radiation_data.csv')

TIME_START = "2023-05-21"
TIME_FINISH = "2023-05-25"

df['time'] = pd.to_datetime(df['time']).dt.tz_localize('UTC').dt.tz_convert(italy_tz)

time_start = italy_tz.localize(pd.to_datetime(TIME_START))
time_finish = italy_tz.localize(pd.to_datetime(TIME_FINISH))

df_filtered = df[(df['time'] >= time_start) & (df['time'] <= time_finish)]

print(f"Dati trovati: {len(df_filtered)} righe nel range {TIME_START} - {TIME_FINISH}")

columns = ['Gb(i)', 'Gd(i)', 'Gr(i)', 'H_sun', 'T2m', 'WS10m', 'time']
variables = ['Gb', 'Gd', 'Gr', 'HSun', 'T', 'WS', 'timestamp']

with open("../sensorPV/resources/real-data/solar-data.c", 'w') as f:
    f.write('#include "solar-data.h"\n\n')
    
    for col, var in zip(columns, variables):
        if col == 'time':
            data_list = [int(dt.timestamp()) for dt in df_filtered[col]]
            chunks = [data_list[i:i+100] for i in range(0, len(data_list), 100)]
            data_str = '{\n' + ',\n'.join(
                ', '.join(map(str, chunk)) for chunk in chunks
            ) + '\n}'
            f.write(f"time_t {var}[] = {data_str};\n\n")
        else:
            data_list = df_filtered[col].tolist()
            chunks = [data_list[i:i+100] for i in range(0, len(data_list), 100)]
            data_str = '{\n' + ',\n'.join(
                ', '.join(map(str, chunk)) for chunk in chunks
            ) + '\n}'
            f.write(f"float {var}[] = {data_str};\n\n")

    f.write('int solar_data_counter = 16;\n\n')




# REAL POWER PW

df = pd.read_csv('dataset/real-powerPV.csv')

df['time'] = pd.to_datetime(df['time']).dt.tz_localize('UTC').dt.tz_convert(italy_tz)

time_start = italy_tz.localize(pd.to_datetime(TIME_START))
time_finish = italy_tz.localize(pd.to_datetime(TIME_FINISH))

df_filtered = df[(df['time'] >= time_start) & (df['time'] <= time_finish)]

print(f"Dati trovati: {len(df_filtered)} righe nel range {TIME_START} - {TIME_FINISH}")

columns = ['time', 'P']
variables = ['timestamp', 'P']

with open("../sensorPV/resources/real-data/power-data.c", 'w') as f:
    f.write('#include "power-data.h"\n\n')
    
    for col, var in zip(columns, variables):
        if col == 'time':
            data_list = [int(dt.timestamp()) for dt in df_filtered[col]]
            chunks = [data_list[i:i+100] for i in range(0, len(data_list), 100)]
            data_str = '{\n' + ',\n'.join(
                ', '.join(map(str, chunk)) for chunk in chunks
            ) + '\n}'
            f.write(f"time_t {var}[] = {data_str};\n\n")
        else:
            data_list = df_filtered[col].tolist()
            chunks = [data_list[i:i+100] for i in range(0, len(data_list), 100)]
            data_str = '{\n' + ',\n'.join(
                ', '.join(map(str, chunk)) for chunk in chunks
            ) + '\n}'
            f.write(f"float {var}[] = {data_str};\n\n")

    f.write('int power_data_counter = 16;\n\n')