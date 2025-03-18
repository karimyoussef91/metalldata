"""
Converts JSON files to Parquet files using Panda.
"""

import argparse
import pandas as pd
import concurrent.futures
import os
import json

def parse_args():
    parser = argparse.ArgumentParser(description='Convert NDJSON files to Parquet files.')
    # i for the input file
    parser.add_argument('-i', type=str, dest='input_file', required=True,
                        help='Input file')
    # o for the output file
    parser.add_argument('-o', type=str, dest='output_file',
                        help='Output file')
    # b for batch size
    parser.add_argument('-b', type=int, dest='batch_size', default=100_000,
                        help='Batch size for parallel generation')
    return parser.parse_args()

def convert_json_to_parquet(input_file, output_file, batch_size):
    with open(input_file) as f:
        data = []
        file_no = 0
        for i, line in enumerate(f):
            data.append(json.loads(line))
            if i > 0 and len(data) % batch_size == 0:
                df = pd.DataFrame(data)
                df.to_parquet(f'{output_file}-{file_no}.parquet')
                file_no += 1
                data.clear()

        if len(data):
            df = pd.DataFrame(data)
            df.to_parquet(f'{output_file}-{file_no}.parquet')


if __name__ == "__main__":
    args = parse_args()
    convert_json_to_parquet(args.input_file, args.output_file, args.batch_size)
