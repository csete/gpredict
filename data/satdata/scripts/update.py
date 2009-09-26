#!/usr/bin/python
#
# This Pythion script is used to update the satellite data in the repository
# with fresh data from CelesTrak.
#
# The script is not needed by end users.
#
# First, the script fetches the TLE data (.txt files) from CelesTrak.
# Then it loads the existing satellites.dat file into memory and replaces
# the TLE1 and TLE2 lines. At the same time, the contents of the .cat files
# are updated by adding new satellites to the .cat files. Satellites are
# not removed from the .cat files, since they could be added by user. The
# tle-new.cat is different: This file will simply contain the same satellites
# as the fresh tle-new.txt file.
#
