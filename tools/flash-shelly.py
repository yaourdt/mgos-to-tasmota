#!/usr/bin/env python3
#  Copyright (c) 2020 Andrew Blackburn & Deomid "rojer" Ryabkov
#  adapted for use with mgos-to-tasmota by Mark Dornbach
#  All rights reserved
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  This script will probe for any shelly device on the network and it will
#  attempt to update them to the latest firmware version available.
#  This script will not flash any firmware to a device that is not already on a
#  version of this firmware, if you are looking to flash your device from stock
#  or any other firmware please follow instructions here:
#  https://github.com/mongoose-os-apps/shelly-homekit/wiki
#
#  Shelly HomeKit flashing script utility
#  usage: flash_shelly.py [-h] [-m {homekit,keep,revert}] [-a] [-l] [-e [EXCLUDE [EXCLUDE ...]]] [-n] [-y] [-V VERSION]
#                         [--variant VARIANT] [-v {0,1}]
#                         [hosts [hosts ...]]
#  positional arguments:
#    hosts
#
#  optional arguments:
#    -h, --help            show this help message and exit
#    -m {homekit,keep,revert}, --mode {homekit,keep,revert}
#                          Script mode.
#    -a, --all             Run against all the devices on the network.
#    -l, --list            List info of shelly device.
#    -e [EXCLUDE [EXCLUDE ...]], --exclude [EXCLUDE [EXCLUDE ...]]
#                          Exclude hosts from found devices.
#    -n, --assume-no       Do a dummy run through.
#    -y, --assume-yes      Do not ask any confirmation to perform the flash.
#    -V VERSION, --version VERSION
#                          Force a particular version.
#    --hap_setup_code HOMEKIT_CODE
#                          Configure HomeKit setup code.
#    --variant VARIANT     Prerelease variant name.
#    -v {0,1}, --verbose {0,1}
#                          Enable verbose logging level.


import argparse
import functools
import json
import logging
import platform
import re
import socket
import subprocess
import sys
import time
import urllib

logging.TRACE = 5
logging.addLevelName(logging.TRACE, 'TRACE')
logging.Logger.trace = functools.partialmethod(logging.Logger.log, logging.TRACE)
logging.trace = functools.partial(logging.log, logging.TRACE)
logging.basicConfig(format='%(message)s')
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

arch = platform.system()
# Windows does not support acsii colours
if not arch.startswith('Win'):
  WHITE = '\033[1m'
  RED = '\033[1;91m'
  GREEN = '\033[1;92m'
  YELLOW = '\033[1;93m'
  BLUE = '\033[1;94m'
  PURPLE = '\033[1;95m'
  NC = '\033[0m'
else:
  WHITE = ''
  RED = ''
  GREEN = ''
  YELLOW = ''
  BLUE = ''
  PURPLE = ''
  NC = ''

def upgrade_pip():
  logger.info("Updating pip...")
  if not arch.startswith('Win'):
    pipe = subprocess.check_output(['python3', '-m', 'pip', 'install', '--upgrade', 'pip'])
  else:
    pipe = subprocess.check_output(['python.exe', '-m', 'pip', 'install', '--upgrade', 'pip'])

try:
  import zeroconf
except ImportError:
  logger.info("Installing zeroconf...")
  upgrade_pip()
  pipe = subprocess.check_output(['pip', 'install', 'zeroconf'])
  import zeroconf
try:
  import requests
except ImportError:
  logger.info("Installing requests...")
  upgrade_pip()
  pipe = subprocess.check_output(['pip', 'install', 'requests'])
  import requests

class MyListener:
  def __init__(self):
    self.device_list = []
    self.p_list = []

  def add_service(self, zeroconf, type, device):
    device = device.replace('._http._tcp.local.', '')
    deviceinfo = Device(device)
    deviceinfo.get_device_info()
    if deviceinfo.fw_type is not None:
      dict = {'host': deviceinfo.host, 'wifi_ip': deviceinfo.wifi_ip, 'fw_type': deviceinfo.fw_type, 'device_url': deviceinfo.device_url, 'info' : deviceinfo.info}
      self.device_list.append(dict)

  def update_service(self, *args, **kwargs):
    pass

class Device:
  def __init__(self, host=None, wifi_ip=None, fw_type=None, device_url=None, info=None, target=None):
    self.host = f'{host}.local' if '.local' not in host and not host[0:3].isdigit() else host
    self.friendly_host = host
    self.fw_type = fw_type
    self.device_url = device_url
    self.info = info
    self.target = target
    self.wifi_ip = wifi_ip
    self.flash_label = "Latest:"

  def is_valid_hostname(self, hostname):
    if len(hostname) > 255:
        result = False
    allowed = re.compile("(?!-)[A-Z\d-]{1,63}(?<!-)$", re.IGNORECASE)
    result = all(allowed.match(x) for x in hostname.split("."))
    logger.trace(f"Valid Hostname: {hostname} {result}")
    return result

  def is_host_online(self, host):
    try:
      self.wifi_ip = socket.gethostbyname(host)
      logger.trace(f"Hostname: {host} is Online")
      return True
    except:
      logger.warning(f"{RED}Could not resolve host: {host}{NC}")
      return False

  def get_device_url(self):
    self.device_url = None
    if self.is_valid_hostname(self.host) and self.is_host_online(self.host):
      try:
        stock_fwcheck = requests.head(f'http://{self.wifi_ip}/settings', timeout=3)
        if stock_fwcheck.status_code == 200:
          self.fw_type = "stock"
          self.device_url = f'http://{self.wifi_ip}/settings'
      except:
        pass
    logger.trace(f"Device URL: {self.device_url}")
    return self.device_url

  def get_device_info(self):
    info = None
    if self.get_device_url():
      try:
        with urllib.request.urlopen(self.device_url) as fp:
          info = json.load(fp)
      except:
        pass
    else:
      logger.debug(f"{RED}Could not get info from device: {self.host}\n{NC}")
    self.info = info
    return info

  def parse_stock_version(self, version):
    # stock version is '20201124-092159/v1.9.0@57ac4ad8', we need '1.9.0'
    if '/v' in version:
      parsed_version = version.split('/v')[1].split('@')[0]
    else:
      parsed_version = '0.0.0'
    return parsed_version

  def get_current_version(self): # used when flashing between firmware versions.
    info = self.get_device_info()
    if not info:
      return None
    if self.fw_type == 'stock':
      version = self.parse_stock_version(info['fw'])
    return version

  def shelly_model(self, type):
    options = {'SHSW-1' : 'Shelly1',
               'SHSW-L' : 'Shelly1L',
               'SHSW-PM' : 'Shelly1PM',
               'SHSW-21' : 'Shelly2',
               'SHSW-25' : 'Shelly25',
               'SHPLG-1' : 'ShellyPlug',
               'SHPLG2-1' : 'ShellyPlug2',
               'SHPLG-S' : 'ShellyPlugS',
               'SHPLG-U1' : 'ShellyPlugUS',
               'SHIX3-1' : 'ShellyI3',
               'SHBTN-1' : 'ShellyButton1',
               'SHBLB-1' : 'ShellyBulb',
               'SHVIN-1' : 'ShellyVintage',
               'SHBDUO-1' : 'ShellyDuo',
               'SHDM-1' : 'ShellyDimmer1',
               'SHDM-2' : 'ShellyDimmer2',
               'SHRGBW2' : 'ShellyRGBW2',
               'SHDW-1' : 'ShellyDoorWindow1',
               'SHDW-2' : 'ShellyDoorWindow2',
               'SHHT-1' : 'ShellyHT',
               'SHSM-01' : 'ShellySmoke',
               'SHWT-1' : 'ShellyFlood',
               'SHGS-1' : 'ShellyGas',
               'SHEM' : 'ShellyEM',
               'SHEM-3' : 'Shelly3EM',
               'switch1' : 'Shelly1',
               'switch1pm' : 'Shelly1PM',
               'switch2' : 'Shelly2',
               'switch25' : 'Shelly25',
               'shelly-plug-s' : 'ShellyPlugS',
               'dimmer1' : 'ShellyDimmer1',
               'dimmer2' : 'ShellyDimmer2',
               'rgbw2' : 'ShellyRGBW2',
               'SHUNI-1': 'ShellyUni',
    }

    return options.get(type, type)

  def update_intermediate(self, release_info=None):
    self.flash_fw_type_str = 'Intermediate'
    self.dlurl = f'http://dl.dasker.eu/firmware/mg2{self.target}-{self.model}.zip'

class StockDevice(Device):
  def get_info(self):
    if not self.info:
      return False
    self.fw_type_str = 'Stock'
    self.fw_version = self.parse_stock_version(self.info['fw'])  # current firmware version
    self.model = self.shelly_model(self.info['device']['type'])
    self.stock_model = self.info['device']['type']
    self.device_id = self.info['mqtt']['id'] if 'id' in self.info['mqtt'] else self.friendly_host
    self.colour_mode = self.info['mode'] if 'mode' in self.info else None
    return True

  def update_to_intermediate(self, release_info=None):
    logger.debug('Mode: Stock To Intermediate')
    self.update_intermediate(release_info)

  def flash_firmware(self):
    logger.info("Now Flashing...")
    dlurl = self.dlurl.replace('https', 'http')
    logger.debug(f"curl -qsS http://{self.wifi_ip}/ota?url={dlurl}")
    response = requests.get(f'http://{self.wifi_ip}/ota?url={dlurl}')
    logger.debug(response.text)

def parse_version(vs):
  # 1.9.2_1L
  # 1.9.3-rc3 / 2.7.0-beta1
  pp = vs.split('_') if '_' in vs else vs.split('-')
  v = pp[0].split('.')
  variant = ""
  varSeq = 0
  if len(pp) > 1:
    i = 0
    for x in pp[1]:
      c = pp[1][i]
      if not c.isnumeric():
        variant += c
      else:
        break
      i += 1
    varSeq = int(pp[1][i]) or 0
  major, minor, patch = [int(e) for e in v]
  return (major, minor, patch, variant, varSeq)

def is_newer(v1, v2):
  vi1 = parse_version(v1)
  vi2 = parse_version(v2)
  if (vi1[0] != vi2[0]):
    return (vi1[0] > vi2[0])
  elif (vi1[1] != vi2[1]):
    return (vi1[1] > vi2[1])
  elif (vi1[2] != vi2[2]):
    return (vi1[2] > vi2[2])
  elif (vi1[3] != vi2[3]):
    return True
  elif (vi1[4] != vi2[4]):
    return (vi1[4] > vi2[4])
  else:
    return False

def write_flash(device_info):
  logger.debug(f"\n{WHITE}write_flash{NC}")
  #flashed = False
  device_info.flash_firmware()
  logger.info(f"please wait for {device_info.friendly_host} to reboot to {device_info.target}.")
  #logger.info(f"waiting for {device_info.friendly_host} to reboot...")
  #time.sleep(3)
  #n = 1
  #waittextshown = False
  #info = None
  #while n < 20:
  #  if n == 10:
  #    logger.info(f"still waiting for {device_info.friendly_host} to reboot...")
  #  onlinecheck = device_info.get_current_version()
  #  time.sleep(1)
  #  n += 1
  #  if onlinecheck == device_info.flash_fw_version:
  #    break
  #  time.sleep(2)
  #if onlinecheck == device_info.flash_fw_version:
  #  logger.info(f"{GREEN}Successfully flashed {device_info.friendly_host} to {device_info.flash_fw_version}{NC}")
  #  if hap_setup_code:
  #    write_hap_setup_code(device_info.wifi_ip, hap_setup_code)
  #else:
  #  if device_info.stock_model == 'SHRGBW2':
  #    logger.info("\nTo finalise flash process you will need to switch 'Modes' in the device WebUI,")
  #    logger.info(f"{WHITE}WARNING!!{NC} If you are using this device in conjunction with Homebridge")
  #    logger.info(f"{WHITE}STOP!!{NC} homebridge before performing next steps.")
  #    logger.info(f"Goto http://{device_info.host} in your web browser")
  #    logger.info("Goto settings section")
  #    logger.info("Goto 'Device Type' and switch modes")
  #    logger.info("Once mode has been changed, you can switch it back to your preferred mode")
  #    logger.info(f"Restart homebridge.")
  #  elif onlinecheck == '0.0.0':
  #    logger.info(f"{RED}Flash may have failed, please manually check version{NC}")
  #  else:
  #    logger.info(f"{RED}Failed to flash {device_info.friendly_host} to {device_info.flash_fw_version}{NC}")
  #  logger.debug("Current: %s" % onlinecheck)

def parse_info(device_info, dry_run, silent_run, target, exclude):
  logger.debug(f"\n{WHITE}parse_info{NC}")
  logger.trace(f"device_info: {device_info}")

  perform_flash = False
  flash = False
  host = device_info.host
  friendly_host = device_info.friendly_host
  device = device_info.device_id
  wifi_ip = device_info.wifi_ip
  current_fw_version = device_info.fw_version
  current_fw_type = device_info.fw_type
  current_fw_type_str = device_info.fw_type_str
  flash_fw_type_str = device_info.flash_fw_type_str
  model = device_info.model
  stock_model = device_info.stock_model
  colour_mode = device_info.colour_mode
  dlurl = device_info.dlurl
  flash_label = device_info.flash_label

  logger.debug(f"host: {host}")
  logger.debug(f"device: {device}")
  logger.debug(f"model: {model}")
  logger.debug(f"stock_model: {stock_model}")
  logger.debug(f"colour_mode: {colour_mode}")
  logger.debug(f"target firmware: {target}")
  logger.debug(f"dlurl: {dlurl}")

  durl_request = requests.get(dlurl, allow_redirects=True) #todo fix this
  if durl_request.status_code != 200:
    #logger.debug(f"status_code: {durl_request.status_code}")
    #logger.debug(f"headers: {durl_request.headers}")
    #logger.debug(f"content: {durl_request.content}")
    latest_fw_label = f"{RED} {device_info.model} not supported{NC}"
    dlurl = None
  else:
    latest_fw_label = "latest"

  logger.info(f"{WHITE}Host: {NC}http://{host}")
  logger.info(f"{WHITE}Device ID: {NC}{device}")
  logger.info(f"{WHITE}IP: {NC}{wifi_ip}")
  logger.info(f"{WHITE}Model: {NC}{model}")
  logger.debug(f"{WHITE}D_URL: {NC}{dlurl}")

  if dry_run == True:
    message = "Would have been"
    keyword = ""
  else:
    message = f"Do you wish to flash"
    keyword = f"{friendly_host} to target {target}"
  if exclude and friendly_host in exclude:
    logger.info("Skipping as device has been excluded...\n")
    return 0
  elif dlurl:
    perform_flash = True
    keyword = "converted to intermediate firmware"
  elif not dlurl:
    keyword = f"{RED}Device is not supported yet...{NC}"
    logger.info(f"{keyword}\n")
    return 0
  else:
    logger.info("Does not need flashing...\n")
    return 0

  logger.debug(f"\nperform_flash: {perform_flash}\n")
  if perform_flash == True and dry_run == False and silent_run == False:
    flash_message = f"Do you wish to flash {friendly_host} to {target}"
    if input(f"{flash_message} (y/n) ? ") == 'y':
      flash = True
    else:
      flash = False
      logger.info("Skipping Flash...")
  elif perform_flash == True and dry_run == False and silent_run == True:
    flash = True
  elif perform_flash == True and dry_run == True:
    logger.info(f"{message} {keyword}...")
  else:
    logger.info(f"{message} {keyword}...")
  if flash == True:
    write_flash(device_info)
  logger.info(" ")

def device_scan(hosts, do_all, dry_run, silent_run, target, exclude):
  logger.debug(f"\n{WHITE}device_scan{NC}")

  if not do_all:
    device_list = []
    logger.info(f"{WHITE}Probing Shelly device for info...\n{NC}")
    for host in hosts:
      deviceinfo = Device(host)
      deviceinfo.get_device_info()
      if deviceinfo.fw_type is not None:
        dict = {'host': deviceinfo.host, 'wifi_ip': deviceinfo.wifi_ip, 'fw_type': deviceinfo.fw_type, 'device_url': deviceinfo.device_url, 'info': deviceinfo.info}
        device_list.append(dict)
  else:
    logger.info(f"{WHITE}Scanning for Shelly devices...{NC}")
    zc = zeroconf.Zeroconf()
    listener = MyListener()
    browser = zeroconf.ServiceBrowser(zc, '_http._tcp.local.', listener)
    zc.wait(100)
    count = 1
    total_loop = 1
    while count < 10:
      nod = len(listener.device_list)
      time.sleep(2)
      count += 1
      if len(listener.device_list) == nod:
        total_loop += 1
      if total_loop > 3:
        break
    zc.close()
    device_list = listener.device_list
    nod = len(device_list)
    logger.info(f"{GREEN}{nod} Devices found.\n{NC}")
  sorted_device_list = sorted(device_list, key=lambda k: k['host'])
  logger.trace(f"device_test: {sorted_device_list}")

  for device in sorted_device_list:
    if device['fw_type'] == 'stock':
      deviceinfo = StockDevice(device['host'], device['wifi_ip'], device['fw_type'], device['device_url'], device['info'], target)
    if not deviceinfo.get_info():
      logger.warning(f"{RED}Failed to lookup local information of {device['host']}{NC}")
      continue

    deviceinfo.update_to_intermediate(intermediate_release_info)

    parse_info(deviceinfo, dry_run, silent_run, target, exclude)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Mongoose to Tasmota, Home Accessory Architect or ESPurna flashing script utility')
  parser.add_argument('-t', '--target', action="store", dest='target', choices=['tasmota', 'haa', 'espurna'], default="tasmota", help="Target firmware.")
  parser.add_argument('-a', '--all', action="store_true", dest='do_all', default=False, help="Run against all the devices on the network.")
  parser.add_argument('-e', '--exclude', action="store", dest="exclude", nargs='*', help="Exclude hosts from found devices.")
  parser.add_argument('-n', '--assume-no', action="store_true", dest='dry_run', default=False, help="Do a dummy run through.")
  parser.add_argument('-y', '--assume-yes', action="store_true", dest='silent_run', default=False, help="Do not ask any confirmation to perform the flash.")
  parser.add_argument('-v', '--verbose', action="store", dest="verbose", choices=['0', '1'], help="Enable verbose logging level.")
  parser.add_argument('hosts', type=str, nargs='*')
  args = parser.parse_args()
  if args.verbose and '0' in args.verbose:
    logger.setLevel(logging.DEBUG)
  elif args.verbose and '1' in args.verbose:
    logger.setLevel(logging.TRACE)

  logger.debug(f"{WHITE}app{NC}")
  logger.debug(f"{PURPLE}OS: {arch}{NC}")
  logger.debug(f"manual_hosts: {args.hosts}")
  logger.debug(f"target: {args.target}")
  logger.debug(f"do_all: {args.do_all}")
  logger.debug(f"dry_run: {args.dry_run}")
  logger.debug(f"silent_run: {args.silent_run}")
  logger.debug(f"exclude: {args.exclude}")
  logger.debug(f"verbose: {args.verbose}")

  if not args.hosts and not args.do_all:
    logger.info(f"{WHITE}Requires a hostname or -a | --all{NC}")
    parser.print_help()
    sys.exit(1)
  elif args.hosts and args.do_all:
    logger.info(f"{WHITE}Invalid option hostname or -a | --all not both.{NC}")
    parser.print_help()
    sys.exit(1)
  try:
    with urllib.request.urlopen("https://api.shelly.cloud/files/firmware") as fp:
      stock_release_info = json.load(fp)
  except urllib.error.URLError as e:
    logger.warning(f"{RED}Failed to lookup online version information{NC}")
    logger.debug(e.reason)
    sys.exit(1)
  try:
    with urllib.request.urlopen("https://rojer.me/files/shelly/update.json") as fp: #todo - replace with own update information
      intermediate_release_info = json.load(fp)
  except urllib.error.URLError as e:
    logger.warning(f"{RED}Failed to lookup online version information{NC}")
    logger.debug(e.reason)
    sys.exit(1)
  logger.trace(f"\n{WHITE}stock_release_info:{NC}{stock_release_info}")
  logger.trace(f"\n{WHITE}intermediate_release_info:{NC}{intermediate_release_info}")

  device_scan(args.hosts, args.do_all, args.dry_run, args.silent_run, args.target, args.exclude)
