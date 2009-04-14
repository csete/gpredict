
WHAT IS GPREDICT?
=================

Gpredict is a real time satellite tracking and orbit prediction program
written using the Gtk+ widgets. Gpredict is targetted mainly towards ham radio
operators but others interested in satellite tracking may find it useful as
well. Gpredict uses the SGP4/SDP4 algorithms, which are compatible with the
NORAD Keplerian elements.


FEATURES
========

Gpredict includes the following features:

    * Tracking an infinite number of satellites only limited by the
      physical memory and processing power of the computer.
    * Display the tracking data in lists, maps, polar plots and any
      combination of these.
    * You can have many modules open at the same either in a
      notebook or in their own windows. The module can also run in
      full-screen mode.
    * You can use many ground stations. Ground station coordinates
      can either be entered manually or you can get some appriximate values
      from a list with more than 2000 predefined locations worldwide.
    * Predict upcoming passes for satellites, including passes where a 
      satellite may be visible and communication windows
    * Very detailed information about both the real time data and the
      predicted passes.
    * Gpredict can run in real-time, simulated real-time (fast forward and
      backward), and manual time control.
    * Doppler tuning of radios via Hamlib rigctld.
    * Antenna rotator control via Hamlib rotctld.

Visit the gpredict homepage at http://gpredcit.oz9aec.net/ for more info.


REQUIREMENTS
============

Gpredict is written using the Gtk+ widget set, which is available for most
Unix like operating systems, Mac and Windows. Following libraries are required
for successful compilation of Gpredict:

- Gtk+ 2.12 or later
- GLib 2.16 or later
- Libcurl 7.16.0 or later
- GooCanvas 0.9 or later
- Hamlib (runtime only, not required for build)

If you compile Gpredict from source you will also need the development parts
of the above mentioned libraries, e.g. gtk+-dev or gtk+-devel and so on.

To install gpredict from source unpack the source package with:

     tar -xvfz gpredict-x.y.z.tar.gz

Change to the gpredict-x.y.z directory and build gpredict:

     ./configure
     make
     make install

The last step usually requires you to become root, otherwise you may not have
the required permissions to install gpredict. If you can not or do not want to
install gpredict as root, you can install gpredict into a custom directory by
adding --prefix=somedir to the ./configure step. For example

  ./configure --prefix=/home/alexc/predict
  
will configure the build to install the files into /home/alexc/gpredict folder.

If the configure step fails with an error, examine the output. It will usually
tell you which package or libraries you need in order to build gpredict. Please
note, that you also need the so-called development packages. In many GNU/Linux
systems you can just install the GNOME Development stuff, but I would recommend
to just install everything if you can (except if you are running Debian ;-). 

If you want to know more about installation options refer to the INSTALL file
(not for beginners).


USING GPREDICT
==============

First time you run gpredict it will start using some default settings. To move on
from that point you can either modify the settings of the default module (click
on the small sown-arrow in the top right corner and select configure), or create
a new module (Menubar->File->New Module). You are highly encouraged to have a look
at the user manual available at

http://gpredict.oz9aec.net/documents.php



LICENSE AND WARRANTY
====================

Gpredict is released under the GNU General Public License and comes with
NO WARRANTY whatsoever (well, maybe except that it works for me). See the
COPYING file for details. If you have problems installing or using Gpredict,
feel free to ask for support. There is a web based forum at
http://forum.oz9aec.net/

Bug trackers, mailing lists, etc, can be accessed at the project page
at sourceforge:
http://sourceforge.net/projects/gpredict


Happy Tracking!

Alexandru Csete
OZ9AEC
