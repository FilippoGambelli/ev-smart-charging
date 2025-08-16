import pandas as pd
import pytz
from datetime import datetime

# Fuso orario italiano
italy_tz = pytz.timezone('Europe/Rome')

# Carica CSV
df = pd.read_csv('dataset/test_radiation_data.csv')

# Intervallo temporale
TIME_START = "2023-05-01"
TIME_FINISH = "2023-05-07"

# Converti la colonna 'time' in datetime
# Se i dati CSV sono in UTC, usare tz_localize('UTC') e poi convertire in Italia
df['time'] = pd.to_datetime(df['time']).dt.tz_localize('UTC').dt.tz_convert(italy_tz)

# Converti TIME_START e TIME_FINISH in datetime aware
time_start = italy_tz.localize(pd.to_datetime(TIME_START))
time_finish = italy_tz.localize(pd.to_datetime(TIME_FINISH))

# Filtra il DataFrame per intervallo temporale
df_filtered = df[(df['time'] >= time_start) & (df['time'] <= time_finish)]

print(f"Dati trovati: {len(df_filtered)} righe nel range {TIME_START} - {TIME_FINISH}")

# Colonne del DataFrame e nomi delle variabili da usare nel file C
columns = ['Gb(i)', 'Gd(i)', 'Gr(i)', 'H_sun', 'T2m', 'WS10m', 'time']
variables = ['Gb', 'Gd', 'Gr', 'HSun', 'T', 'WS', 'timestamp']

with open("../sensorPV/resources/solar-data/solar-data.c", 'w') as f:
    f.write('#include "solar-data.h"\n\n')
    
    for col, var in zip(columns, variables):
        if col == 'time':
            # Converti in time_t (secondi dall'epoch, ora italiana)
            data_list = [int(dt.timestamp()) for dt in df_filtered[col]]
            data_str = '{' + ', '.join(map(str, data_list)) + '}'
            f.write(f"time_t {var}[] = {data_str};\n\n")
        else:
            data_list = df_filtered[col].tolist()
            data_str = '{' + ', '.join(map(str, data_list)) + '}'
            f.write(f"float {var}[] = {data_str};\n\n")

    f.write('int counter = 0;\n\n')