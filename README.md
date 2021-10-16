# SurfBeam2
ViaSat SurfBeam2 satellite modem

**SurfBeam2** is an open source CGI parser and UI application for ViaSat SurfBeam2 satellite modems. 

It relies on the strings available from two URLs
  - 192.168.100.1/index.cgi?page=modemStatusData   (indoor unit)
  - 192.168.100.1/index.cgi?page=triaStatusData    (outdoor unit)

which are split into substrings, the extracted content being converted in human readable units (dB, dBm, % etc.). 

**Important** The number of expected substrings from both URLs, as well as the meaning of a specific position index is firmware version dependent. More information about how they are decoded is provided in the header file. As far as the author is aware, there is no officially documented CGI packet arrangement, therefore firmware versions different than the supported one may lead to different substring counts, as well as different index meanings for some of the substrings.
