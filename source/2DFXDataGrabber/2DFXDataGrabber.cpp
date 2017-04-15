#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <iomanip>
using namespace std;

enum BlinkTypes
{
	DEFAULT,
	RANDOM_FLASHING,
	T_1S_ON_1S_OFF,
	T_2S_ON_2S_OFF,
	T_3S_ON_3S_OFF,
	T_4S_ON_4S_OFF,
	T_5S_ON_5S_OFF,
	T_6S_ON_4S_OFF
};

string Filename;

void ConvertDFFTo2DFX()
{
	SetCurrentDirectory(".\\2dfx\\");
	WIN32_FIND_DATA fd;
	HANDLE dffFile = FindFirstFile("*.dff", &fd);
	if (dffFile != INVALID_HANDLE_VALUE)
	{
		static int i = 0;
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

				unsigned int pos = 5;
				while (fd.cFileName[pos])
					++pos;
				if (fd.cFileName[pos - 4] == '.' && (fd.cFileName[pos - 3] == 'd' || fd.cFileName[pos - 3] == 'D') &&
					(fd.cFileName[pos - 2] == 'f' || fd.cFileName[pos - 2] == 'F') && (fd.cFileName[pos - 1] == 'f' || fd.cFileName[pos - 1] == 'F'))
				{
					ifstream DFFContent(fd.cFileName, ios::in | ios::binary);
					string ext = fd.cFileName; ext.replace(ext.find("dff"), 3, "2dfx");
					std::string str((std::istreambuf_iterator<char>(DFFContent)), std::istreambuf_iterator<char>());
					size_t pos = str.find("coronastar");
					if (pos != string::npos)
					{
						if ((*(unsigned int*)(str.c_str() + (pos - 49))) > 117)
						{
							i = 0;
							do
							{
								i++;
								pos -= 1;
								std::cout << *(unsigned int*)(str.c_str() + (pos - 49)) << std::endl;
								if (i > 100)
									break;

							} while ((*(unsigned int*)(str.c_str() + (pos - 49))) < 2 || (*(unsigned int*)(str.c_str() + (pos - 49))) > 15);
						}

						ofstream out2DFX(ext, ios::out | ios::binary);
						out2DFX.write(str.c_str() + (pos - 49), str.size() - (pos - 49));
						out2DFX.close();
					}
					DFFContent.close();
					//DeleteFile(fd.cFileName);
				}
			}

		} while (FindNextFile(dffFile, &fd));
		FindClose(dffFile);
	}

	SetCurrentDirectory("..\\");

	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = ".\\SA2dfx.exe";
	ShExecInfo.lpParameters = "";
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOWNORMAL;
	ShExecInfo.hInstApp = NULL;
	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);

	ofstream LodLights;
	LodLights.open(Filename, fstream::trunc);

	LodLights << "#%modelname" << endl;
	LodLights << "#Red, Green, Blue, Alpha, OffsetX, OffsetY, OffsetZ, CustomSize, BlinkType, NoDistance, DrawSearchlight" << endl;
	LodLights << "#0.45 - custom size reserved for traffic lights" << endl;
	LodLights << endl;

	SetCurrentDirectory(".\\2dfx\\");

	HANDLE TXTFile = FindFirstFile("*.txt", &fd);
	if (TXTFile != INVALID_HANDLE_VALUE)
	{
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

				unsigned int pos = 5;
				while (fd.cFileName[pos])
					++pos;
				if (fd.cFileName[pos - 4] == '.' && (fd.cFileName[pos - 3] == 't' || fd.cFileName[pos - 3] == 'T') &&
					(fd.cFileName[pos - 2] == 'x' || fd.cFileName[pos - 2] == 'X') && (fd.cFileName[pos - 1] == 't' || fd.cFileName[pos - 1] == 'T'))
				{
					if (LodLights.is_open())
					{
						ifstream TXTFileContent(fd.cFileName); string buf;
						string ext = fd.cFileName; ext.replace(ext.find(".txt"), 4, "\0");
						LodLights << "%" << ext.c_str() << endl;

						string EntryName; float offsetX, offsetY, offsetZ, CoronaSize;	int R = 0, G = 0, B = 0, A = 255; int BlinkType = 0; int nNoDistance = 0; int nDrawSearchlight = 0;
						while (getline(TXTFileContent, buf))
						{
							if (buf[0] != '#')
							{
								if (sscanf(buf.c_str(), "%s", &EntryName[0]))
								{
									if (buf.find("Position") != std::string::npos)
									{
										sscanf(buf.c_str(), "%*s %f %f %f", &offsetX, &offsetY, &offsetZ);
									}
									else if (buf.find("Color") != std::string::npos)
									{
										sscanf(buf.c_str(), "%*s %d %d %d %*d", &R, &G, &B);
									}
									else if (buf.find("CoronaSize") != std::string::npos)
									{
										sscanf(buf.c_str(), "%*s %f", &CoronaSize);

										if (ext.find("lamppost1") != std::string::npos || ext.find("lamppost2") != std::string::npos ||
										ext.find("lamppost3") != std::string::npos || ext.find("mlamppost") != std::string::npos ||
										ext.find("vegaslampost") != std::string::npos || ext.find("vegaslampost2") != std::string::npos ||
										ext.find("streetlamp1") != std::string::npos || ext.find("streetlamp2") != std::string::npos ||
										ext.find("ce_radarmast3") != std::string::npos || ext.find("radarmast1_lawn") != std::string::npos ||
										ext.find("radarmast1_lawn01") != std::string::npos || ext.find("bollardlight") != std::string::npos ||
										ext.find("sub_floodlite") != std::string::npos || ext.find("cj_traffic_light5") != std::string::npos ||
										ext.find("lamppost_coast") != std::string::npos || ext.find("airuntest_las") != std::string::npos ||
										ext.find("vegasairportlight") != std::string::npos || ext.find("gay_lamppost") != std::string::npos ||
										ext.find("gay_traffic_light") != std::string::npos || ext.find("cj_traffic_light4") != std::string::npos ||
										ext.find("lodnsmitter_sfs01") != std::string::npos || ext.find("lodrailbge01_sfse") != std::string::npos ||
										ext.find("lodrailbge02_sfse") != std::string::npos || ext.find("lodrailbge04_sfse") != std::string::npos ||
										ext.find("lodrailbge09_sfse") != std::string::npos || ext.find("lodtainercrane_04") != std::string::npos ||
										ext.find("lod_refchimny01") != std::string::npos || ext.find("mtraffic1") != std::string::npos ||
										ext.find("mtraffic2") != std::string::npos || ext.find("mtraffic3") != std::string::npos ||
										ext.find("mtraffic4") != std::string::npos || ext.find("trafficlight1") != std::string::npos ||
										ext.find("vgsstriptlights1") != std::string::npos || ext.find("circuslampost03") != std::string::npos)
										{
											nDrawSearchlight = 1;
										}
										
										if (ext.find("cj_traffic_light3") != std::string::npos || ext.find("cj_traffic_light4") != std::string::npos ||
										ext.find("cj_traffic_light5") != std::string::npos || ext.find("gay_traffic_light") != std::string::npos ||
										ext.find("mtraffic1") != std::string::npos || ext.find("mtraffic2") != std::string::npos ||
										ext.find("mtraffic4") != std::string::npos || ext.find("trafficlight1") != std::string::npos ||
										ext.find("vgsstriptlights1") != std::string::npos || ext.find("traincross1") != std::string::npos)
										{
											CoronaSize = 0.45f;
											nDrawSearchlight = 0;
											if ((R == 255 && G == 148 && B == 52) || (R == 255 && G == 255 && B == 221))
												CoronaSize = 1.0f;
										}

										if (CoronaSize > 1.0f)
											CoronaSize = 1.2f;

										(offsetX == -0.0f) ? offsetX = 0.0f : offsetX;
										(offsetY == -0.0f) ? offsetY = 0.0f : offsetY;
										(offsetZ == -0.0f) ? offsetZ = 0.0f : offsetZ;
									}
									else if (buf.find("CoronaShowMode") != std::string::npos)
									{
										if (buf.find("DEFAULT") != std::string::npos || buf.find("TRAFFICLIGHT") != std::string::npos ||
											buf.find("TRAINCROSSLIGHT") != std::string::npos || buf.find("AT_RAIN_ONLY") != std::string::npos)
										{
											BlinkType = BlinkTypes::DEFAULT;
										}
										else if (buf.find("RANDOM_FLASHING") != std::string::npos || buf.find("RANDOM_FLASHIN_ALWAYS_AT_WET_WEATHER") != std::string::npos ||
												 buf.find("ALWAYS_AT_WET_WEATHER") != std::string::npos)
										{
											BlinkType = BlinkTypes::DEFAULT; //BlinkType = BlinkTypes::RANDOM_FLASHING;
										}
										else if (buf.find("LIGHTS_ANIM_SPEED_4X") != std::string::npos)
										{
											BlinkType = BlinkTypes::T_1S_ON_1S_OFF;
										}
										else if (buf.find("LIGHTS_ANIM_SPEED_2X") != std::string::npos)
										{
											BlinkType = BlinkTypes::T_2S_ON_2S_OFF;
										}
										else if (buf.find("LIGHTS_ANIM_SPEED_1X") != std::string::npos)
										{
											BlinkType = BlinkTypes::T_4S_ON_4S_OFF;
										}
										else if (buf.find("5S_ON_5S_OFF") != std::string::npos)
										{
											BlinkType = BlinkTypes::T_5S_ON_5S_OFF;
										}
										else if (buf.find("6S_ON_4S_OFF") != std::string::npos || buf.find("6S_ON_4S_OFF_2") != std::string::npos)
										{
											BlinkType = BlinkTypes::T_6S_ON_4S_OFF;
										}

										if (CoronaSize > 0.0f)
										{
											auto GetPrecison = [](float value) -> int
											{
												int width = 10;
												int digits = 0;

												if (value < 0.0f)
												{
													value *= -1.0f;
													digits++;
												}
												digits += ((value<1) ? 1 : int(1 + log10(float(abs(value)))));
												int precision = (((width - digits - 1) >= 0) ? (width - digits - 1) : 0);
												return precision;
											};

											LodLights << std::right
												<< std::setfill('0')
												<< std::setw(3) << R << " "
												<< std::setw(3) << G << " "
												<< std::setw(3) << B << " "
												<< std::setw(3) << A << " "
												<< std::setfill(' ')
												<< std::left
												<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetX)) << offsetX << " "
												<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetY)) << offsetY << " "
												<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetZ)) << offsetZ << " "
												<< std::setw(4) << std::fixed << std::setprecision(2) << CoronaSize << " "
												<< std::right << std::setfill('0')
												<< std::setw(2) << BlinkType << " "
												<< std::left << std::setfill(' ')
												<< std::setw(1) << nNoDistance << " "
												<< std::setw(1) << nDrawSearchlight
												<< std::endl;
										}
									}
								}
							}
						}
						TXTFileContent.close();
					}
				}
			}

		} while (FindNextFile(TXTFile, &fd));
		FindClose(TXTFile);
	}

	LodLights << "%lodnsmitter_sfs01" << endl;
	LodLights << "255 000 000 255 -9.2980000 -11.402000 -68.624001 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 10.5050000 -0.0400000 -68.624001 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -9.2120000 11.2440000 -68.624001 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -7.9260000 9.12600000 -33.070000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 7.93200000 -0.0020000 -33.070000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -7.9740000 -9.2030000 -33.070000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -6.8400000 7.28500000 0.82600000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -6.8120000 -7.4080000 0.82600000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 5.76800000 -0.0970000 0.82600000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 7.37700000 0.06100000 24.6870000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -7.5860000 -8.5990000 24.6870000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -7.9060000 8.64800000 24.6870000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -7.8930000 15.8600000 51.5480000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -13.689000 12.5180000 51.5480000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 13.9000000 3.25600000 51.7970010 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 13.6740000 -3.2850000 51.7970010 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -7.9330000 -15.856000 51.6160010 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -13.234000 -12.272000 51.5480000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -8.1140000 -9.2560000 89.3089980 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 8.03700000 -0.0150000 89.3089980 1.00 00 1 0" << endl;
	LodLights << "%radarmast1_lawn" << endl;
	LodLights << "255 000 000 0.091000000000 0.05300000 24.6200000 0.10 00 1 0" << endl;
	LodLights << "%radarmast1_lawn01" << endl;
	LodLights << "255 000 000 255 0.13000000 -0.0080000 24.4900000 1.00 00 1 0" << endl;
	LodLights << "%ce_radarmast3" << endl;
	LodLights << "255 000 000 255 0.00100000 0.03300000 32.6570000 0.50 00 1 0" << endl;
	LodLights << "%ws_ref_bollard" << endl;
	LodLights << "255 000 000 255 0.00900000 0.00100000 0.46900000 0.70 00 1 0" << endl;
	LodLights << "%baybridge1_sfse" << endl;
	LodLights << "000 111 255 255 -76.679000 -16.446000 -8.7210000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -53.127000 -16.434000 -5.5900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -29.320000 -16.434000 -0.1430000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -6.1820000 -16.434000 7.34300000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 16.3480000 -16.434000 16.7610000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 37.8660000 -16.434000 28.0760000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 58.7830000 -16.434000 41.4320000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -76.679000 16.4680000 -8.7210000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -53.127000 16.4680000 -5.5900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -29.320000 16.4900000 -0.1430000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -6.1820000 16.4900000 7.34300000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 16.3480000 16.4900000 16.7610000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 37.8660000 16.4900000 28.0760000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 58.7830000 16.4900000 41.4320000 1.20 00 1 0" << endl;
	LodLights << "%baybridge2_sfse" << endl;
	LodLights << "255 000 000 255 38.4845000 -18.216500 83.4015000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 38.4790000 18.3384000 83.4015000 1.00 00 1 0" << endl;
	LodLights << "000 111 255 255 74.8720000 16.4080000 39.9210000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 74.8720000 -16.476000 39.9210000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 56.6360000 -16.476000 56.2430000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 56.6360000 16.4080000 56.2430000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 17.5910000 -16.476000 53.3240000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 17.5910000 16.4080000 53.3240000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -0.5510000 -16.476000 37.6250000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -0.5510000 16.4080000 37.6250000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -19.959000 -16.476000 24.1330000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -19.959000 16.4080000 24.1330000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -40.829000 -16.476000 12.1530000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -62.489000 16.4080000 2.28400000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -62.489000 -16.476000 2.26900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -84.947000 -16.476000 -5.7150000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -84.947000 16.4080000 -5.7010000 1.20 00 1 0" << endl;
	LodLights << "%baybridge3_sfse" << endl;
	LodLights << "255 000 000 255 -100.83000 18.3671000 89.5265000 1.00 00 1 0" << endl;
	LodLights << "255 000 000 255 -100.87000 -18.140600 89.5265000 1.00 00 1 0" << endl;
	LodLights << "000 111 255 255 136.609000 -16.542000 -5.2150000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 136.609000 16.3830000 -5.2150000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 112.756000 16.3830000 -8.1900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 112.756000 -16.542000 -8.1900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 69.1490000 16.3830000 -8.1900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 69.1490000 -16.542000 -8.1900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 45.4190000 -16.542000 -4.9990000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 45.4190000 16.3830000 -4.9990000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 22.1890000 -16.542000 0.42900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 22.1890000 16.3830000 0.42900000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -0.2570000 -16.542000 8.42400000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -0.2570000 16.3830000 8.42400000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -21.660000 16.3830000 18.3810000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -21.660000 -16.542000 18.3810000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -42.686000 -16.542000 30.2490000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -42.686000 16.3830000 30.2490000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -62.074000 -16.421000 43.8980000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -62.074000 16.5040000 43.8980000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -80.451000 -16.463000 59.1700000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -80.451000 16.4620000 59.1700000 1.20 00 1 0" << endl;
	LodLights << "%baybridge4_sfse" << endl;
	LodLights << "000 111 255 255 73.9600000 16.4190000 66.5760000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 73.9600000 -16.496000 66.5760000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 55.5380000 16.4190000 50.2920000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 55.5380000 -16.496000 50.2920000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 36.2830000 16.4190000 35.6030000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 36.2830000 -16.496000 35.6030000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 15.3910000 16.4190000 22.1260000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 15.3910000 -16.496000 22.1260000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -6.2090000 16.4190000 10.8280000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -6.2090000 -16.496000 10.8280000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -28.574000 16.4190000 1.46500000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -28.574000 -16.496000 1.46500000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -51.905000 16.4190000 -5.8690000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -51.905000 -16.496000 -5.8690000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -75.652000 16.4190000 -11.285000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -75.652000 -16.496000 -11.285000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -99.500000 16.4190000 -14.214000 1.20 00 1 0" << endl;
	LodLights << "000 111 255 255 -99.500000 -16.496000 -14.214000 1.20 00 1 0" << endl;
	LodLights << "%ggbrig_06_sfw" << endl;
	LodLights << "255 000 000 255 19.0970000 0.12800000 115.890000 2.00 00 1 0" << endl;
	LodLights << "%ggbrig_01_sfw" << endl;
	LodLights << "255 000 000 255 19.0970000 0.12800000 115.890000 2.00 00 1 0" << endl;
	LodLights << "%ggbrig_03_sfw" << endl;
	LodLights << "255 000 000 255 19.3850000 55.7600000 19.4330000 1.20 00 1 0" << endl;
	LodLights << "255 000 000 255 -19.136000 55.7600000 19.4330000 1.20 00 1 0" << endl;
	LodLights << "255 000 000 255 -19.136000 -5.9890000 -30.831000 1.20 00 1 0" << endl;
	LodLights << "255 000 000 255 19.3850000 -5.9890000 -30.831000 1.20 00 1 0" << endl;
	LodLights << "%ggbrig_05_sfw" << endl;
	LodLights << "255 000 000 255 -19.308000 -59.193000 17.7280000 1.20 00 1 0" << endl;
	LodLights << "255 000 000 255 19.1360000 -59.193000 17.7280000 1.20 00 1 0" << endl;
	LodLights << "255 000 000 255 19.1360000 2.37500000 -31.452000 1.20 00 1 0" << endl;
	LodLights << "255 000 000 255 -19.308000 2.37500000 -31.452000 1.20 00 1 0" << endl;
	LodLights << "%cxrf_whitebrig" << endl;
	LodLights << "249 145 34 255 -0.11800000 -29.064000 5.17500000 1.20 00 1 0" << endl;
	LodLights << "249 145 34 255 16.63200000 -29.064000 5.17500000 1.20 00 1 0" << endl;
	LodLights << "249 145 34 255 16.63200000 28.9200000 5.17500000 1.20 00 1 0" << endl;
	LodLights << "249 145 34 255 -0.11800000 28.9200000 5.17500000 1.20 00 1 0" << endl;
	LodLights << "%des_bigtelescope" << endl;
	LodLights << "255 000 000 255 24.0510000 24.2580000 53.9150000 3.00 00 1 0" << endl;
	LodLights << "%pylon_big1_" << endl;
	LodLights << "255 000 000 255 4.67100000 0.13400000 36.0030000 1.20 00 1 0" << endl;
	LodLights << "255 000 000 255 -4.3560000 0.13400000 36.0030000 1.20 00 1 0" << endl;

	LodLights << "%additional_coronas" << endl;
	LodLights << "255 255 255 255 -651.28700 2145.91000 74.7152000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -781.25600 2140.42000 74.7152000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -811.16800 2015.78000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -794.20900 2024.26000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -776.24400 2031.43000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -733.82300 2041.57000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -714.85400 2043.30000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -695.27600 2043.66000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -653.37600 2038.31000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -635.24900 2032.18000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 255 255 255 -617.31900 2024.71000 57.1071000 0.70 00 1 0" << endl;
	LodLights << "255 000 000 255 -750.89400 2036.27000 76.7071000 0.70 00 1 0" << endl;
	LodLights << "255 000 000 255 -678.84900 2040.47000 76.7071000 0.70 00 1 0" << endl;
	LodLights << "#place you coronas here" << endl;

	LodLights.close();
	SetCurrentDirectory("..\\");
}

void ConvertLightTo2DFX()
{
	ofstream LodLights;
	LodLights.open(Filename, fstream::trunc);

	LodLights << "#%modelname" << endl;
	LodLights << "#Red, Green, Blue, Alpha, OffsetX, OffsetY, OffsetZ, CustomSize, BlinkType, NoDistance, DrawSearchlight" << endl;
	LodLights << "#0.45 - custom size reserved for traffic lights" << endl;
	LodLights << endl;

	SetCurrentDirectory(".\\iv\\");
	WIN32_FIND_DATA fd;
	HANDLE TXTFile = FindFirstFile("*.light", &fd);
	if (TXTFile != INVALID_HANDLE_VALUE)
	{
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
			{
				
				ifstream TXTFileContent(fd.cFileName); string buf;
				string ext = fd.cFileName; ext.replace(ext.find(".light"), 6, "\0");
				string ext2 = ext;
				//LodLights << "%" << ext.c_str() << endl;
				bool mdlname = false;

				if (ext2.find("bm_nylamp110b") != std::string::npos || ext2.find("bm_nylamp1b") != std::string::npos || ext2.find("bm_nylamp2b") != std::string::npos ||
					ext2.find("cj_nylamp2b") != std::string::npos)
					continue;

				string EntryName; float offsetX, offsetY, offsetZ, CoronaSize;	int R = 0, G = 0, B = 0, A = 255; int BlinkType = 0; int nNoDistance = 0; int nDrawSearchlight = 0;
				while (getline(TXTFileContent, buf))
				{
					if (buf[0] != '#')
					{
						if (sscanf(buf.c_str(), "%s", &EntryName[0]))
						{
							if (buf.find("Position") != std::string::npos)
							{
								sscanf(buf.c_str(), "%*s %f %f %f", &offsetX, &offsetY, &offsetZ);
							}
							else if (buf.find("Color") != std::string::npos)
							{
								sscanf(buf.c_str(), "%*s %d %d %d %*d", &R, &G, &B);
							}
							else if (buf.find("CoronaSize") != std::string::npos)
							{
								sscanf(buf.c_str(), "%*s %f", &CoronaSize);

								if (CoronaSize > 1.0f)
									CoronaSize = 1.2f;

								if (ext.find("traf") != std::string::npos)
									CoronaSize = 0.45f;

								(offsetX == -0.0f) ? offsetX = 0.0f : offsetX;
								(offsetY == -0.0f) ? offsetY = 0.0f : offsetY;
								(offsetZ == -0.0f) ? offsetZ = 0.0f : offsetZ;
							}
							else if (buf.find("Type Spot") != std::string::npos || buf.find("Type Omni") != std::string::npos)
							{
								ext = ext2 + ".skel";
								ifstream TXTFileContent2(ext); string buf2;
								bool br = false;
								if (TXTFileContent2.is_open())
								{
									while (getline(TXTFileContent2, buf2))
									{
										if (offsetZ >= 21.0f)
											break;
										if (buf2.find("LocalOffset") != std::string::npos)
										{
											float offsetX2, offsetY2, offsetZ2;
											sscanf(buf2.c_str(), "%*s %f %f %f", &offsetX2, &offsetY2, &offsetZ2);
											offsetX += offsetX2;
											offsetY += offsetY2;
											offsetZ += offsetZ2;
										}

										//if (buf2.find("NumBones 0") != std::string::npos || buf2.find("NumBones 1") != std::string::npos)
										//{
										//	br = true;
										//	break;
										//}
									}
									TXTFileContent2.close();
								}
								//else
								//	continue;

								//if (br)
								//	continue;

								if (CoronaSize > 0.0f)
								{
									auto GetPrecison = [](float value) -> int
									{
										int width = 10;
										int digits = 0;

										if (value < 0.0f)
										{
											value *= -1.0f;
											digits++;
										}
										digits += ((value<1) ? 1 : int(1 + log10(float(abs(value)))));
										int precision = (((width - digits - 1) >= 0) ? (width - digits - 1) : 0);
										return precision;
									};

									if (!mdlname)
									{
										LodLights << "%" << ext2.c_str() << endl;
										mdlname = true;
									}

									LodLights << std::right
										<< std::setfill('0')
										<< std::setw(3) << R << " "
										<< std::setw(3) << G << " "
										<< std::setw(3) << B << " "
										<< std::setw(3) << ((buf.find("Type Omni") != std::string::npos) ? 128 : A) << " "
										<< std::setfill(' ')
										<< std::left
										<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetX)) << offsetX << " "
										<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetY)) << offsetY << " "
										<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetZ)) << offsetZ << " "
										<< std::setw(4) << std::fixed << std::setprecision(2) << CoronaSize << " "
										<< std::right << std::setfill('0')
										<< std::setw(2) << BlinkType << " "
										<< std::left << std::setfill(' ')
										<< std::setw(1) << nNoDistance << " "
										<< std::setw(1) << nDrawSearchlight
										<< std::endl;
								}
							}

						}
					}
				}
				TXTFileContent.close();
			}

		} while (FindNextFile(TXTFile, &fd));
		FindClose(TXTFile);
	}

	LodLights << "%bm_nylamp110b" << endl;
	LodLights << "255 219 155 255 -1.1508710 -0.0219250 6.95975000 0.80 00 0 0" << endl;
	LodLights << "255 219 155 255 1.10668200 -0.0219260 6.95970100 0.80 00 0 0" << endl;
	LodLights << "%bm_nylamp1b" << endl;
	LodLights << "255 221 163 255 -1.6176590 -0.0347459 7.78166600 0.80 00 0 0" << endl;
	LodLights << "255 221 163 255 1.58855900 -0.0347870 7.78166800 0.80 00 0 0" << endl;
	LodLights << "%bm_nylamp2b" << endl;
	LodLights << "255 221 163 255 -2.5801490 -0.0306550 7.68636900 0.80 00 0 0" << endl;
	LodLights << "255 221 163 255 2.56053300 -0.0384880 7.68502200 0.80 00 0 0" << endl;
	LodLights << "%cj_nylamp2b" << endl;
	LodLights << "255 221 163 255 -2.1766780 0.12343800 8.0072020 0.80 00 0 0" << endl;
	LodLights << "255 221 163 255 2.21655300 0.12538500 8.0055760 0.80 00 0 0" << endl;

	LodLights.close();
	SetCurrentDirectory("..\\");
}

int _tmain(int argc, _TCHAR* argv[])
{
	string line, line2;

	ifstream GTAData;
	GTAData.open(".\\gta4.dat", std::ifstream::in);
	if (GTAData.is_open())
	{
		GTAData.close();
		Filename = "IVLodLights.dat";
		ConvertLightTo2DFX();
		return 0;
	}

	GTAData.open(".\\gta3.dat", std::ifstream::in);
	Filename = "IIILodLights.dat";
	if (!GTAData.is_open())
	{
		GTAData.open(".\\gta_vc.dat", std::ifstream::in);
		Filename = "VCLodLights.dat";
		if (!GTAData.is_open())
		{
			Filename = "SALodLights.dat";
			ConvertDFFTo2DFX();
			return 0;
		}
	}

	
	fstream IDEData;
	fstream FX2DData;

	IDEData.open(".\\IDEData.txt", ofstream::trunc | fstream::out);
	FX2DData.open(".\\2DFXData.txt", ofstream::trunc | fstream::out);

	while (getline(GTAData, line))
	{
		if (line[0] != '#')
		{
			if (line[0] == 'I' && line[1] == 'D' && line[2] == 'E')
			{
				cout << line.c_str() + 9 << endl;
				string IDEline;
				ifstream IDEFile(line.c_str() + 9);
				if (IDEFile.is_open())
				{
					while (getline(IDEFile, IDEline))
					{
						if (IDEline[0] != '#')
						{
							int ID; string ModelName;
							IDEline.erase(std::remove(IDEline.begin(), IDEline.end(), ','), IDEline.end());
							if (sscanf(IDEline.c_str(), "%d %s %*s %*d %*s", &ID, &ModelName[0]))
							{
								if (IDEline.find("coronastar") == std::string::npos)
								{
									IDEData << IDEline << endl;
								}
								else
								{
									FX2DData << IDEline << endl;
								}
							}
						}
					}
				}
			}
		}
	}
	GTAData.close();
	IDEData.close();
	FX2DData.close();


	ofstream LodLights;
	LodLights.open(Filename, fstream::trunc);
	IDEData.open("IDEData.txt", std::ifstream::in);
	FX2DData.open("2DFXData.txt", std::ifstream::in);
	if (FX2DData.is_open())
	{
		LodLights << "#%modelname" << endl;
		LodLights << "#Red, Green, Blue, Alpha, OffsetX, OffsetY, OffsetZ, CustomSize, BlinkType, NoDistance, DrawSearchlight" << endl;
		LodLights << "#0.45 - custom size reserved for traffic lights" << endl;
		LodLights << endl;

		static int PrevID;
		static int TrafficLCount;
		while (getline(FX2DData, line))
		{
			int ID; float offsetX, offsetY, offsetZ; int R = 0, G = 0, B = 0, A = 255; int BlinkType = 0; int nNoDistance = 0; int nDrawSearchlight = 0; int unk1, type; string Corona, Shadow; float Distance, OuterRange, CoronaSize, InnerRange; int ShadowIntensity, Wet, Flare, Dust;
			line.erase(std::remove(line.begin(), line.end(), ','), line.end());
			if (sscanf(line.c_str(), "%d %f %f %f %d %d %d %d %d %s %s %f %f %f %f %d %d %d %d %d",
			&ID, &offsetX, &offsetY, &offsetZ, &R, &G, &B, &unk1, &type, &Corona[0], &Shadow[0], &Distance, &OuterRange, &CoronaSize, &InnerRange, &ShadowIntensity, &BlinkType, &Wet, &Flare, &Dust))
			{	
				if (BlinkType == 0 || BlinkType == 1 || BlinkType == 12)
				{
					BlinkType = BlinkTypes::DEFAULT;
				}
				else if (BlinkType == 2 || BlinkType == 3 || BlinkType == 10 || BlinkType == 11)
				{
					BlinkType = BlinkTypes::DEFAULT; //BlinkType = BlinkTypes::RANDOM_FLASHING;
				}
				else if (BlinkType == 4 || BlinkType == 5 || BlinkType == 13)
				{
					BlinkType = BlinkTypes::T_1S_ON_1S_OFF;
				}
				else if (BlinkType == 6 || BlinkType == 7)
				{
					BlinkType = BlinkTypes::T_2S_ON_2S_OFF;
				}
				else if (BlinkType == 8 || BlinkType == 9)
				{
					BlinkType = BlinkTypes::T_3S_ON_3S_OFF;
				}

				if (CoronaSize > 1.0f)
					CoronaSize = 1.2f;

				if (!CoronaSize || ID == 4481 || ID == 4483) //destruct2 destruct03
					continue;

				auto GetPrecison = [](float value) -> int
				{
					int width = 10;
					int digits = 0;

					if (value < 0.0f)
					{
						value *= -1.0f;
						digits++;
					}
					digits += ((value<1) ? 1 : int(1 + log10(float(abs(value)))));
					int precision = (((width - digits - 1) >= 0) ? (width - digits - 1) : 0);
					return precision;
				};

				if (PrevID != ID)
				{
					IDEData.clear();
					IDEData.seekg(0, ios::beg);
					while (getline(IDEData, line2))
					{
						line2.erase(std::remove(line2.begin(), line2.end(), ','), line2.end());
						int IdeID; string ModelName2;
						if (sscanf(line2.c_str(), "%d %s %*s %*d %*s", &IdeID, &ModelName2[0]))
						{
							if (IdeID == ID)
							{
								LodLights << "%" << ModelName2.c_str() << endl;
								break;
							}
						}
					}
					std::transform(line2.begin(), line2.end(), line2.begin(), ::tolower);

					if (line2.find("lamppost1") != std::string::npos || line2.find("lamppost2") != std::string::npos ||
						line2.find("lamppost3") != std::string::npos || line2.find("sub_floodlite") != std::string::npos ||
						line2.find("mlamppost") != std::string::npos || line2.find("doublestreetlght1") != std::string::npos ||
						line2.find("bollardlight") != std::string::npos || line2.find("lampost_coast") != std::string::npos)
					{
						nDrawSearchlight = 1;
					}

					if (line2.find("mtraffic1") != std::string::npos || line2.find("mtraffic2") != std::string::npos ||
						line2.find("mtraffic4") != std::string::npos || line2.find("trafficlight1") != std::string::npos)
					{
						CoronaSize = 0.45f;
						nDrawSearchlight = 0;
						R = 255;
						G = 0;
						B = 0;
						TrafficLCount = 0;
					}
					
					(offsetX == -0.0f) ? offsetX = 0.0f : offsetX;
					(offsetY == -0.0f) ? offsetY = 0.0f : offsetY;
					(offsetZ == -0.0f) ? offsetZ = 0.0f : offsetZ;

					LodLights << std::right
						<< std::setfill('0')
						<< std::setw(3) << R << " "
						<< std::setw(3) << G << " "
						<< std::setw(3) << B << " "
						<< std::setw(3) << A << " "
						<< std::setfill(' ')
						<< std::left
						<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetX)) << offsetX << " "
						<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetY)) << offsetY << " "
						<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetZ)) << offsetZ << " "
						<< std::setw(4) << std::fixed << std::setprecision(2) << CoronaSize << " "
						<< std::right << std::setfill('0')
						<< std::setw(2) << BlinkType << " "
						<< std::left << std::setfill(' ')
						<< std::setw(1) << nNoDistance << " "
						<< std::setw(1) << nDrawSearchlight
						<< std::endl;

					PrevID = ID;
				}
				else
				{
					std::transform(line2.begin(), line2.end(), line2.begin(), ::tolower);

					if (line2.find("lamppost1") != std::string::npos || line2.find("lamppost2") != std::string::npos ||
						line2.find("lamppost3") != std::string::npos || line2.find("sub_floodlite") != std::string::npos ||
						line2.find("mlamppost") != std::string::npos || line2.find("doublestreetlght1") != std::string::npos ||
						line2.find("bollardlight") != std::string::npos || line2.find("lampost_coast") != std::string::npos)
					{
						nDrawSearchlight = 1;
					}

					if (line2.find("mtraffic1") != std::string::npos || line2.find("mtraffic2") != std::string::npos ||
						line2.find("mtraffic4") != std::string::npos || line2.find("trafficlight1") != std::string::npos)
					{
						CoronaSize = 0.45f;
						nDrawSearchlight = 0;

						TrafficLCount++;
						switch (TrafficLCount)
						{
						case 1:
							R = 255;
							G = 128;
							B = 0;
							break;
						case 2:
							R = 0;
							G = 255;
							B = 0;
							break;
						case 3:
							R = 255;
							G = 0;
							B = 0;
							break;
						case 4:
							R = 255;
							G = 128;
							B = 0;
							break;
						case 5:
							R = 0;
							G = 255;
							B = 0;
							break;
						default:
							break;
						}
					}

					(offsetX == -0.0f) ? offsetX = 0.0f : offsetX;
					(offsetY == -0.0f) ? offsetY = 0.0f : offsetY;
					(offsetZ == -0.0f) ? offsetZ = 0.0f : offsetZ;

					LodLights << std::right
						<< std::setfill('0')
						<< std::setw(3) << R << " "
						<< std::setw(3) << G << " "
						<< std::setw(3) << B << " "
						<< std::setw(3) << A << " "
						<< std::setfill(' ')
						<< std::left
						<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetX)) << offsetX << " "
						<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetY)) << offsetY << " "
						<< std::setw(10) << std::fixed << std::setprecision(GetPrecison(offsetZ)) << offsetZ << " "
						<< std::setw(4) << std::fixed << std::setprecision(2) << CoronaSize << " "
						<< std::right << std::setfill('0')
						<< std::setw(2) << BlinkType << " "
						<< std::left << std::setfill(' ')
						<< std::setw(1) << nNoDistance << " "
						<< std::setw(1) << nDrawSearchlight
						<< std::endl;
				}
			}
		}
	}

	LodLights << "%additional_coronas" << endl;
	if (Filename == "IIILodLights.dat")
	{
		LodLights << "255 000 000 255 225.823000 -1554.6000 461.855000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 277.323000 -1530.9000 461.855000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 289.423000 -1434.6000 461.855000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 276.523000 -1411.9000 461.855000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 216.623000 -1411.9000 461.855000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 206.723000 -1429.2000 461.855000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 207.223000 -1475.6000 461.855000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 262.954000 -1353.9700 298.755000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 262.958000 -1303.7200 298.755000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 223.264000 -1303.6700 298.755000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 223.256000 -1353.9200 298.755000 0.70 00 1 0" << endl;
		LodLights << "255 145 000 255 100.484000 -1365.0000 416.956000 2.00 00 1 0" << endl;
		LodLights << "255 000 000 255 400.353000 -328.01500 187.855000 1.00 00 1 0" << endl;
		LodLights << "255 000 000 255 26.6703000 -1161.4900 177.355000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 -5.4137900 -1146.4000 176.955000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 -39.829600 -1216.0000 176.955000 0.70 00 1 0" << endl;
		LodLights << "255 000 000 255 26.5036000 -1247.6300 177.355000 0.70 00 1 0" << endl;
		LodLights << "255 255 255 255 460.366000 -951.66900 48.3555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 460.366000 -908.26900 48.3555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 486.166000 -908.36900 56.2555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 486.166000 -951.76900 56.2555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 509.166000 -908.36900 66.2555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 509.166000 -951.76900 66.2555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 531.566000 -908.36900 77.7555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 531.566000 -951.76900 77.7555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 554.466000 -908.36900 93.9555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 554.466000 -951.76900 93.9555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 577.366000 -908.36900 114.356000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 577.366000 -951.76900 114.356000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 599.566000 -951.76900 139.155000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 599.566000 -908.36900 139.155000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 771.866000 -952.66900 48.3555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 771.366000 -909.26900 48.3555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 744.966000 -909.36900 56.7555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 745.466000 -952.76900 56.7555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 722.666000 -952.56900 65.6555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 722.166000 -909.16900 65.6555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 700.166000 -952.56900 78.2555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 699.666000 -909.16900 78.2555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 678.066000 -952.46900 93.7555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 677.566000 -909.06900 93.7555000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 654.466000 -952.26900 113.856000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 653.966000 -908.86900 113.855000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 632.466000 -952.46900 139.155000 1.20 00 1 0" << endl;
		LodLights << "255 255 255 255 631.966000 -909.06900 139.155000 1.20 00 1 0" << endl;
		LodLights << "#place you coronas here" << endl;
	}
	else
	{
		LodLights << "255 000 255 255 -682.82100 -919.10200 12.5548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -662.82100 -920.20200 13.7548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -642.82100 -920.80200 15.2548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -622.82100 -921.00200 16.8548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -682.82100 -939.70200 12.5548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -662.82100 -940.60200 13.7548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -642.82100 -941.20200 15.2548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -622.82100 -941.40200 16.8548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -602.82100 -921.10200 18.2548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -602.82100 -941.50200 18.2548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -582.82100 -921.20200 19.7548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -582.82100 -941.60200 19.7548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -562.82100 -921.20200 21.5548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -562.82100 -941.60200 21.5548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -542.82100 -921.20200 22.6548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -542.82100 -941.60200 22.6548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -522.82100 -921.20200 23.3548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -522.82100 -941.60200 23.3548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -502.82100 -941.60200 24.1548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -502.82100 -921.20200 24.1548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -482.82100 -921.60200 24.4548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -482.82100 -942.00200 24.4548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -462.82100 -922.10200 24.4548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -462.82100 -942.50200 24.4548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -442.82100 -922.40200 23.9548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -442.82100 -942.80200 23.9548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -422.82100 -922.70200 23.3548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -422.82100 -943.10200 23.3548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -402.82100 -943.50200 22.4548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -402.82100 -923.10200 22.4548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -382.82100 -944.00200 21.1548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -382.82100 -923.60200 21.1548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -362.82100 -944.70200 19.5548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -362.82100 -924.30200 19.5548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -342.82100 -945.20200 18.2548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -342.82100 -924.80200 18.2548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -322.82100 -925.90200 17.0548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -322.82100 -946.40200 17.0548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -302.82100 -927.00200 15.9548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -302.82100 -947.50200 15.9548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -282.82100 -948.30200 15.2548000 0.70 00 1 0" << endl;
		LodLights << "255 000 255 255 -282.82100 -927.90200 15.2548000 0.70 00 1 0" << endl;
		LodLights << "#place you coronas here" << endl;
	}

	LodLights.close();

	//std::cout << "Press ENTER to exit." << std::endl;
	//cin.ignore();
	return 0;
}

