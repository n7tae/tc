// Copyright © 2022 Thomas A. Early N7TAE
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <iostream>
#include <fstream>
#include <iomanip>
#include <future>
#include <unistd.h>
#include <cmath>

#include "TCPacketDef.h"
#include "UnixDgramSocket.h"

uint32_t TCReader(const char *ofname)
{
	std::string name(TC2REF);
	name.append(1, 'A');
	CUnixDgramReader Reader;
	if (Reader.Open(name.c_str()))
	{
		std::cerr << "Could not open " << name << std::endl;
		return 0;
	}

	const std::string of(ofname);
	std::ofstream raw(of+".raw", std::ofstream::binary | std::ofstream::trunc);
	std::ofstream dst(of+".dst", std::ofstream::binary | std::ofstream::trunc);
	std::ofstream dmr(of+".dmr", std::ofstream::binary | std::ofstream::trunc);
	std::ofstream m17(of+".m17", std::ofstream::binary | std::ofstream::trunc);
	if (! raw.is_open() || ! dst.is_open() || ! dmr.is_open() || ! m17.is_open())
	{
		std::cerr << "could not open the output file(s)" << std::endl;
		return 0;
	}

	STCPacket packet;
	packet.is_last = false;
	int samples = 0;
	int max = 0;
	int min = 0;
	double ss = 0.0;
	while( ! packet.is_last)
	{
		Reader.Read(&packet);
		for (int i=0; i<160; i++)
		{
			auto a = packet.audio[i];
			ss += pow(double(a), 2.0);
			if (a > max) max = a;
			if (a < min) min = a;
		}
		samples += 160;
		raw.write((const char *)packet.audio, 320);
		dst.write((const char *)packet.dstar, 9);
		dmr.write((const char *)packet.dmr, 9);
		if (packet.sequence % 2 || packet.is_last)
			m17.write((const char *)packet.m17, 16);
	}
	Reader.Close();
	// we don't need one of these files
	raw.close(); if (packet.codec_in == ECodecType::audio  ) unlink((of+".raw").c_str());
	dst.close(); if (packet.codec_in == ECodecType::dstar  ) unlink((of+".dst").c_str());
	dmr.close(); if (packet.codec_in == ECodecType::dmr    ) unlink((of+".dmr").c_str());
	m17.close(); if (packet.codec_in == ECodecType::c2_3200) unlink((of+".m17").c_str());

	// the reference value of a sine wave with amplitude 32767
	double ref = 0.0;
	for (int i=0; i<160; i++)
	{
		auto a = 32767.0 * sin(M_PI*double(i)/160.0);
		ref += a * a;
	}
	ref = 20.0 * log10(sqrt(ref/160.0));
	double v = 20.0 * log10(sqrt(ss/samples)) - ref;
	std::cout << of << " reference gain = " << v << " dB min:max = " << min << ":" << max << std::endl;
	return packet.sequence;
}

int main(int argc, char *argv[])
{
	if (3 != argc)
	{
		std::cerr << "usage:" << argv[0] << " inputfile, outputbasefile" << std::endl;
	}

	// figure out what's coming in and set up the packet
	const std::string infile(argv[1]);
	STCPacket packet;
	if (std::string::npos != infile.find(".raw"))
		packet.codec_in = ECodecType::audio;
	else if (std::string::npos != infile.find(".dst"))
		packet.codec_in = ECodecType::dstar;
	else if (std::string::npos != infile.find(".dmr"))
		packet.codec_in = ECodecType::dmr;
	else if (std::string::npos != infile.find(".m17"))
		packet.codec_in = ECodecType::c2_3200;
	else
	{
		std::cerr << "Don't know how to deal with " << infile <<  std::endl;
		return EXIT_FAILURE;
	}
	packet.is_last = false;
	packet.module = 'A';
	packet.sequence = 0;
	packet.streamid = 0xabcdu;

	// open the input file
	std::ifstream ifile(infile, std::ifstream::binary);
	if (! ifile.is_open())
	{
		std::cerr << "Can't open " << argv[1] << std::endl;
		return EXIT_FAILURE;
	}

	// open the transcoder
	CUnixDgramWriter tcWriter;
	tcWriter.SetUp(REF2TC);

	// start the read thread
	auto fut = std::async(std::launch::async, &TCReader, argv[2]);

	while (! ifile.eof())
	{
		switch (packet.codec_in)
		{
		case ECodecType::audio:
			memset(packet.audio, 0, 320);
			ifile.read((char *)packet.audio, 320);
			break;
		case ECodecType::dstar:
			ifile.read((char *)packet.dstar, 9);
			break;
		case ECodecType::dmr:
			ifile.read((char *)packet.dmr, 9);
			break;
		case ECodecType::c2_3200:
			ifile.read((char *)packet.m17, 16);
			break;
		}
		packet.rt_timer.start();
		if (ECodecType::c2_3200 == packet.codec_in)
		{
			tcWriter.Send(&packet);
			packet.sequence++;
		}
		if (ifile.eof())
			packet.is_last = true;
		tcWriter.Send(&packet);
		packet.sequence++;
	}
	return EXIT_SUCCESS;
}
