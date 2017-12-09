#!/usr/bin/python
#
# This script is used to create the initial satellite data repository by
# converting Celestrak TLE files into a satellites.dat and .cat files.
#
# 1. Copy this script into a temp folder
# 2. Create subdirectories ./in/ ./tmp/ and ./out/
# 3. Run the script
# 4. You should have a .cat file for each category as well as a satellites.dat
#    file in the ./out/ folder
#
# This script is only used during development and is not needed by end users.
#
import os
import string
import urllib

# Satellite groups
groups = {
    "amateur" : "Amateur Radio",
    "cubesat" : "Cubesat",
    "galileo" : "Galileo Nav.",
    "glo-ops" : "Glonass Operational",
    "gps-ops" : "GPS Operational",
    "iridium" : "Iridium",
    "iridium-NEXT" : "Iridium NEXT",
    "molniya" : "Molniya",
    "noaa" : "NOAA",
    "science" : "Space & Earth Science",
    "visual" : "Brightest",
    "weather" : "Weather Satellites"
}

# Satellite nicknames
nicknames = {
    "4321" : "AO-5",
    "6236" : "AO-6",
    "7530" : "AO-7",
    "10703" : "AO-8",
    "14129" : "AO-10",
    "14781" : "UO-11",
    "16909" : "FO-12",
    "20437" : "UO-14",
    "20438" : "UO-15",
    "20439" : "AO-16",
    "20440" : "DO-17",
    "20441" : "WO-18",
    "20442" : "LO-19",
    "20480" : "FO-20",
    "21087" : "RS-14",
    "21089" : "RS-12/13",
    "21575" : "UO-22",
    "22077" : "KO-23",
    "22654" : "AO-24",
    "22825" : "AO-27",
    "22826" : "IO-26",
    "22828" : "KO-25",
    "22829" : "PO-28",
    "23439" : "RS-15",
    "24278" : "FO-29",
    "24305" : "MO-30",
    "25396" : "TO-31",
    "25397" : "GO-32",
    "25509" : "SO-33",
    "25520" : "PO-34",
    "25544" : "ISS",
    "25636" : "SO-35",
    "25693" : "UO-36",
    "26063" : "OO-38",
    "26545" : "SO-41",
    "26548" : "MO-46",
    "26549" : "SO-42",
    "26609" : "AO-40",
    "26931" : "NO-44",
    "26932" : "NO-45",
    "27607" : "SO-50",
    "27844" : "CO-55",
    "27848" : "CO-57",
    "27939" : "RS-22",
    "28375" : "AO-51",
    "28650" : "VO-52",
    "28894" : "XO-53",
    "28895" : "CO-58",
    "28941" : "CO-56",
    "35870" : "SO-67",
    "36122" : "HO-68",
    "39444" : "AO-73",
    "40967" : "AO-85",
    "43017" : "AO-91"
}

urlprefix = "http://celestrak.com/NORAD/elements/"


for group, name in groups.iteritems():
    webfile = urlprefix + group + ".txt"
    localfile = "./in/" + group + ".txt"
    print "Fetching " + webfile + " => " + localfile
    urllib.urlretrieve (webfile, localfile)

### CHK ###

# For each input file
for group, name in groups.iteritems():

    print name+':'

    # open TLE file for reading
    tlefile = open('./in/'+group+'.txt', 'r')

    # create category file
    category = "./out/"+group+".cat"
    catfile = open(category, 'w')

    # first line is the group name
    catfile.write(name+'\n')

    while 1:
        # read three lines at a time; strip trailing whitespaces
        line1 = tlefile.readline().strip()
        if not line1: break
        line2 = tlefile.readline().strip()
        line3 = tlefile.readline().strip()

        # catalog number; strip leading zeroes
        catnum = line2[2:7].lstrip('0')
        print " ... "+catnum

        # add satellite to category
        catfile.write(catnum+'\n')

        # satellite file
        satfile = open('./tmp/'+catnum+'.sat','w')
        
        satfile.write('VERSION=1.1\n')
        satfile.write('NAME='+line1+'\n')
        if catnum in nicknames:
            satfile.write('NICKNAME='+nicknames[catnum]+'\n')
        else:
            satfile.write('NICKNAME='+line1+'\n')
        satfile.write('TLE1='+line2+'\n')
        satfile.write('TLE2='+line3+'\n')

        satfile.close()

    # close TLE and CAT files
    tlefile.close()
    catfile.close()

# now package the .sat files into one .dat file
datfile = open('./out/satellites.dat', 'w')
for file in os.listdir("./tmp/"):

    # open .sat file
    satfile = open("./tmp/"+file, "r")

    # Create fake config group of catnum
    catnum = file.partition(".")[0]
    datfile.write("\n["+catnum+"]\n")

    # read lines from satfile and write them to datfile
    text = satfile.readlines()
    datfile.writelines(text)

    satfile.close();

datfile.close()

