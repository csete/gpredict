#!/usr/bin/python
#
# This script was used to convert Celestrak TLE files into individual .sat and .cat files
#
import os
import string


# For each input file
for file in os.listdir("./in/"):

    print file

    # open TLE file for reading
    tlefile = open('./in/'+file, 'r')

    # create category file
    category = "./out/"+file.partition(".")[0]+".cat"
    catfile = open(category, 'w')

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

