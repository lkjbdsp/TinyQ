#!/usr/bin/python

# this script will update the versions in plist and installer files to match that in resource.h

import plistlib, os, datetime, fileinput, glob, sys, string
scriptpath = os.path.dirname(os.path.realpath(__file__))

def replacestrs(filename, s, r):
  files = glob.glob(filename)
  
  for line in fileinput.input(files,inplace=1):
    line.find(s)
    line = line.replace(s, r)
    sys.stdout.write(line)

    
def readPlist(path):
    with open(path, "rb") as f:
        return plistlib.load(f)

def writePlist(data, path):


    # plistlib.dump in modern Python accepts fmt=plistlib.FMT_XML / FMT_BINARY
    fmt_const = plistlib.FMT_XML
    with open(path, "wb") as f:
        plistlib.dump(data, f)
    
def main():

  MajorStr = ""
  MinorStr = "" 
  BugfixStr = ""

  for line in fileinput.input(scriptpath + "/resource.h",inplace=0):
    if "#define PLUG_VER " in line:
      FullVersion = int(line.lstrip("#define PLUG_VER "), 16)      
      major = FullVersion & 0xFFFF0000
      MajorStr = str(major >> 16)
      minor = FullVersion & 0x0000FF00
      MinorStr = str(minor >> 8)
      BugfixStr = str(FullVersion & 0x000000FF)
      
  
  FullVersionStr = MajorStr + "." + MinorStr + "." + BugfixStr
  
  today = datetime.date.today()
  CFBundleGetInfoString = FullVersionStr + ", Copyright lkjb, " + str(today.year)
  CFBundleVersion = FullVersionStr
  
  print("update_version.py - setting version to " + FullVersionStr)
  print("Updating plist version info...")
  
  plistpath = scriptpath + "/resources/TinyQ-VST2-Info.plist"
  vst2 = readPlist(plistpath)
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  writePlist(vst2, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  plistpath = scriptpath + "/resources/TinyQ-AU-Info.plist"
  au = readPlist(plistpath)
  au['CFBundleGetInfoString'] = CFBundleGetInfoString
  au['CFBundleVersion'] = CFBundleVersion
  au['CFBundleShortVersionString'] = CFBundleVersion
  writePlist(au, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  plistpath = scriptpath + "/resources/TinyQ-VST3-Info.plist"
  vst3 = readPlist(plistpath)
  vst3['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst3['CFBundleVersion'] = CFBundleVersion
  vst3['CFBundleShortVersionString'] = CFBundleVersion
  writePlist(vst3, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  print("Updating Mac Installer version info...")
  
  plistpath = scriptpath + "/installer/TinyQ.pkgproj"
  installer = readPlist(plistpath)
  
  for x in range(0,6):
    installer['PACKAGES'][x]['PACKAGE_SETTINGS']['VERSION'] = FullVersionStr
  
  writePlist(installer, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  print("Updating Windows Installer version info...")
  
  for line in fileinput.input(scriptpath + "/installer/TinyQ.iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + FullVersionStr + "\n"
    sys.stdout.write(line)

if __name__ == '__main__':
  main()