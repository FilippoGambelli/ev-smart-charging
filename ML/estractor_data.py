import pandas as pd
from datetime import datetime

df = pd.read_csv('dataset/test_radiation_data.csv')

TIME_START = "2023-05-01"
TIME_FINISH = "2023-05-07"

df['time'] = pd.to_datetime(df['time'])

df_filtered = df[(df['time'] >= TIME_START) & (df['time'] <= TIME_FINISH)]

print(f"Dati trovati: {len(df_filtered)} righe nel range {TIME_START} - {TIME_FINISH}")

# Colonne del DataFrame e nomi delle variabili da usare nel file
columns = ['Gb(i)', 'Gd(i)', 'Gr(i)', 'H_sun', 'T2m', 'WS10m']
variables = ['Gb', 'Gd', 'Gr', 'Hsun', 'T', 'WS']

with open("../sensorPV/resources/solar-data/sensor-data.c", 'w') as f:
    f.write('#include "solar-data.h"\n\n')
    
    for col, var in zip(columns, variables):
        data_list = df_filtered[col].tolist()

        data_str = '{' + ', '.join(map(str, data_list)) + '}'
        f.write(f"float {var}[] = {data_str};\n\n")

    f.write('int counter = 0; \n\n')    
