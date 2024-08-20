import pandas as pd


df = pd.read_parquet("parquet_dataset/data1.parquet")
print("="*10, "parquet_dataset/data1.parquet", "="*10)
print(df)

df = pd.read_parquet("parquet_dataset/data2.parquet")
print("="*10, "parquet_dataset/data2.parquet", "="*10)
print(df)
