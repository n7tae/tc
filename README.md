# tc - a tool for transcoding digital voice data

## Introduction

This is a tool that was developed to investigate the possible differences in audio gain produced by differnet digital voice encoders. With each successful transcoding, tc will report the audio volume as reference (either input or output). The reference audio is a sine wave of amplitude 32767, *i.e.*, a fully saturated, 16-bit sine wave. Therefore the reported dB "gain" will be negative and for best results in the real world, should about 10-20 dB below zero.

## Description

This is a simple tool that will translate between:

- Signed, 16-bit 8000 Hz audio
- AMBE2, used by D-Star
- AMBE2Plus, used by DMR and Yaesu System Fusion
- Codec2-3200, used by the M17 Project

It uses a specially modified tcd running as a systemd service. Simply point tc to a raw file of audio or encoded audio and specify the base name of the output files and tc will create the three new audio files. File detection type is forced by the input file extension:

- .raw is audio data
- .dst is D-Star data
- .dmr is DMR/YSF data
- .m17 is Codec2-3200 data

## Prerequites

The special tcd service uses DVSI-based hardware for AMBE transcoding. You need a pair of DVSI-3000 or DVSI-3003 devices. These devices use FTDI USB-to-serial chips to communcation with the DVSI vocoders, so download and install the latest D2XX drive package from the [FTDI web site](https://ftdichip.com/drivers/d2xx-drivers/).

## Compile and Install

Clone this repository:

```bash
git clone https://github.com/n7tae/tc.git
```

Move to the new repo and comile:

```bash
cd tc
make
```

Now install the transcoding service

```bash
sudo make install
```

Note that because of the FTDI library, this service runs at root privledge.

The program tc will do the work. Try it on an example audio file:

```bash
./tc test.raw frmAudio
```

This will create three encoded files: `frmAudio.dst`, `frmAudio.dmr` and `frmAudio.m17`. You can run any one of these output files through tc again and it will produce the other three files:

```bash
./tc frmAudio.m17 frmM17 # makes frmM17.raw, frmM17.dst and frmM17.dmr
```

There is also a shell script `demo` that will use aplay to play the original test.raw file and three more audio files that have been encoded to the three data formats and then decoded back to audio. You can also use arecord to create you own .raw audio file (16-bit little endian, single channel, 8000 Hz) and then use tc to produce the encoded data files.

You may need to install aplay and arecord:

```bash
sudo apt alsa-utils
```
