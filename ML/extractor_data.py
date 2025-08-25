import pandas as pd

# Time range for filtering
TIME_START = "2023-05-15 09:00:00"
TIME_FINISH = "2023-05-16 17:00:00"

def load_and_filter_csv(file_path, time_col='time'):
    df = pd.read_csv(file_path)

    # Converte solo in datetime, senza timezone
    df[time_col] = pd.to_datetime(df[time_col])

    time_start = pd.to_datetime(TIME_START)
    time_finish = pd.to_datetime(TIME_FINISH)

    df_filtered = df[(df[time_col] >= time_start) & (df[time_col] <= time_finish)]

    return df_filtered


def write_c_array(file_path, columns, variables, df_filtered, chunk_size=100, header_file=None, counter_value=None, counter_name=None):
    with open(file_path, 'w') as f:
        if header_file:
            f.write(f'#include "{header_file}"\n\n')
        
        for col, var in zip(columns, variables):
            data_list = df_filtered[col].tolist() if col != 'time' else [int(dt.timestamp()) for dt in df_filtered[col]]
            
            # Split data into chunks for readability
            chunks = [data_list[i:i+chunk_size] for i in range(0, len(data_list), chunk_size)]
            data_str = '{\n' + ',\n'.join(', '.join(map(str, chunk)) for chunk in chunks) + '\n}'
            
            c_type = 'static const time_t' if col == 'time' else 'static const float'
            f.write(f"{c_type} {var}[] = {data_str};\n\n")
        
        time_var = variables[columns.index('time')]
        f.write(f"#define {time_var.upper()}_LEN (sizeof({time_var})/sizeof({time_var}[0]))\n\n")
        
        if counter_value is not None and counter_name is not None:
            f.write(f'extern int {counter_name};\n')

# --- SOLAR DATA ---
solar_df = load_and_filter_csv('dataset/radiation_data_20sec.csv')
print(f"Solar data found: {len(solar_df)} rows in range {TIME_START} - {TIME_FINISH}")

solar_columns = ['Gb(i)', 'Gd(i)', 'Gr(i)', 'H_sun', 'T2m', 'WS10m', 'time']
solar_variables = ['Gb', 'Gd', 'Gr', 'HSun', 'T', 'WS', 'solar_data_timestamp']

write_c_array(
    file_path="../sensorPV/resources/real-data/solar-data.h",
    columns=solar_columns,
    variables=solar_variables,
    df_filtered=solar_df,
    header_file="time.h",
    counter_value=360,
    counter_name="solar_data_counter"
)


# --- REAL POWER DATA ---
power_df = load_and_filter_csv('dataset/real_power_data_5sec.csv')
print(f"Power data found: {len(power_df)} rows in range {TIME_START} - {TIME_FINISH}")

power_columns = ['time', 'P']
power_variables = ['power_data_timestamp', 'P']

write_c_array(
    file_path="../sensorPV/resources/real-data/power-data.h",
    columns=power_columns,
    variables=power_variables,
    df_filtered=power_df,
    header_file="time.h",
    counter_value=1440,
    counter_name="power_data_counter"
)

# DEBUG PRINTS: check timestamps at counter indices; they should match for proper alignment
print(f"Solar data time at counter index 360: {solar_df.iloc[360]['time']}")
print(f"Power data time at counter index 1440: {power_df.iloc[1440]['time']}")