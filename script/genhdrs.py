#!/usr/bin/env python
import argparse
import re
import sys

parser = argparse.ArgumentParser('genhdrs.py')
parser.add_argument('-c', '--config', help="Config file")
parser.add_argument('-d', '--default', help="Default configuration", action="store_true")
parser.add_argument('-D', '--default-file', help="Default configuration file", default="defconf")
parser.add_argument('-g', '--get', help="Get a specific value of config")
parser.add_argument('-o', '--output', help="Generate header file")

def parse_size(size: str) -> int:
    """
    return the number of blocks given by size: *, *k/K/kiB/KiB, *M/MiB, *G/GiB
    """
    size_regex = r'^\d+(([kKMG]i?)(B|Hz)?)?$'

    size_match = re.match(size_regex, size)

    if size_match is None:
        raise Exception('Invalid disk size')
    
    match_groups = size_match.groups() # example: ('kiB', 'k', 'iB'), ('k', 'k', None), (None, None, None)
    multiplier = 1

    # SI prefixes
    if match_groups[2] != None:
        if match_groups[1] == 'k' or match_groups[1] == 'K':
            multiplier = 1000

        if match_groups[1] == 'M':
            multiplier = 1000 * 1000

        if match_groups[1] == 'G':
            multiplier = 1000 * 1000 * 1000
            
        if match_groups[1] == 'ki' or match_groups[1] == 'Ki':
            multiplier = 1024

        if match_groups[1] == 'Mi':
            multiplier = 1024 * 1024

        if match_groups[1] == 'Gi':
            multiplier = 1024 * 1024 * 1024
            
    else:
        if match_groups[1] == 'k' or match_groups[1] == 'K' or match_groups[1] == 'ki' or match_groups[1] == 'Ki':
            multiplier = 1024

        if match_groups[1] == 'M' or match_groups[1] == 'Mi':
            multiplier = 1024 * 1024

        if match_groups[1] == 'G' or match_groups[1] == 'Gi':
            multiplier = 1024 * 1024 * 1024

    if match_groups[0]:
        size = size[:-len(match_groups[0])]

    size_num = int(size) * multiplier
    return size_num

def load_file(kv_map: dict[str, str], filename: str) -> None:
    with open(filename, 'r') as f:
        for line in f:
            line = line.split("#")[0]
            line = line.strip()
            if line == '':
                continue
            line_syntax = r'^[A-Za-z0-9\-_]+\s*?=\s*?\S+$'
            if not re.match(line_syntax, line):
                print("Unsupported syntax", file=sys.stderr)
            symbol, val = [i.strip() for i in line.split("=")]
            kv_map[symbol] = val
            
def override_file(kv_map: dict[str, str], filename: str) -> None:
    with open(filename, 'r') as f:
        for line in f:
            line = line.split("#")[0]
            line = line.strip()
            if line == '':
                continue
            line_syntax = r'^[A-Za-z0-9\-_]+\s*?=\s*?\S+$'
            if not re.match(line_syntax, line):
                print("Unsupported syntax", file=sys.stderr)
            symbol, val = [i.strip() for i in line.split("=")]     
            
            if symbol not in kv_map.keys():
                print(f"Symbol {symbol} is not a valid symbol in config", file=sys.stderr);
                exit(-1)
            kv_map[symbol] = val


def main():
    args = parser.parse_args()
    if args.default and args.config or not args.default and not args.config:
        print("Please specify either default by -d,--default or config", file=sys.stderr)
        exit(-1)
    # load default
    kv_map: dict[str, str] = {}
    load_file(kv_map, args.default_file)
    
    if args.config:
        override_file(kv_map, args.config)
    if args.get:
        if args.get in kv_map.keys():
            print(kv_map[args.get])
        else:
            print("Unable to get variable", file=sys.stderr)
            exit(-1)
    if args.output:
        with open(args.output, 'w') as f:
            preprocessor_val = f"_K_{args.output.split('/')[-1].upper().replace('.', '_')}"
            f.writelines([f"#ifndef {preprocessor_val}\n",
                          f"#define {preprocessor_val}\n", "\n"])
            f.writelines([f"#define {key} {hex(parse_size(kv_map[key]))}\n" for key in kv_map.keys()])
            f.writelines(["\n", "#endif\n"])
        
if __name__ == "__main__":
    main()