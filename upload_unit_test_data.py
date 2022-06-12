#!/usr/bin/env python3
import argparse
from string import Template
import json
import sys
import subprocess

def get_args():
    p = argparse.ArgumentParser()
    p.add_argument("fnamein", help="input filename of template json file")
    p.add_argument("-o", "--fnameout", default="data/unit_tests.json", help="output filename; default is data/unit_tests.json")
    p.add_argument("-s", "--ssid", required=True, help="Wifi SSID name")
    p.add_argument("-p", "--pwd", required=True, help="Wifi password")
    p.add_argument("-u", "--upload", action="store_true", default=False, help="upload to esp8266")
    args = p.parse_args()
    return args

def load_template(fname):
    with open(fname,"rt") as f:
        lines = f.read()
    #
    return lines

def fill_template(templ, ssid, pwd):
    t = Template(templ)
    s = t.substitute({"SSID" : ssid,
                      "PWD": pwd})
    return s

def verify_json_validity(json_txt):
    try:
        j = json.loads(json_txt)
    except  json.JSONDecodeError as e:
        print(f"Invalid JSON: {e.msg} in {e.pos}:{e.lineno}")
        sys.exit(1)
    print("JSON is valid")
    return

def upload_data_dir():
    #cmd = ["dpio","run","-t","uploadfs"]
    #cmd = ["source", "aliases.sh", "&&","declare","-f","dpio"]
    cmd = ". ./aliases.sh && dpio run -t uploadfs"
    subprocess.run(cmd,shell=True)
    

if __name__ == "__main__":
    args = get_args()

    templ = load_template(args.fnamein)
    filled = fill_template(templ, args.ssid, args.pwd)
    verify_json_validity(filled)

        
    #
    # write the file
    with open(args.fnameout,"wt") as f:
        f.write(filled)
    print("done")
    
    if args.upload==True:
        upload_data_dir()
    #
    
