#include "stdafx.h"
#include "windows.h"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <iomanip>
using namespace std;

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
	LodLights << "#coronaColour.r, coronaColour.g, coronaColour.b, vecOffset.x, vecOffset.y, vecOffset.z, fCustomSize" << endl;
	LodLights << "#coronaColour2.r, coronaColour2.g, coronaColour2.b, vecOffset2.x, vecOffset2.y, vecOffset2.z, fCustomSize2" << endl;
	LodLights << "#%modelname2" << endl;
	LodLights << "#coronaColour.r, coronaColour.g, coronaColour.b, vecOffset.x, vecOffset.y, vecOffset.z, fCustomSize" << endl;
	LodLights << "#%additional_coronas" << endl;
	LodLights << "#coronaColour.r, coronaColour.g, coronaColour.b, position.x, position.y, position.z, fCustomSize, 1" << endl;
	LodLights << "#place you coronas here" << endl;
	LodLights << "#0.45 - reserved for traffic lights" << endl;
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

						string EntryName; float offsetX, offsetY, offsetZ, CoronaSize;	int R, G, B;
						while (getline(TXTFileContent, buf))
						{
							if (buf[0] != '#')
							{
								if (sscanf(buf.c_str(), "%s", &EntryName))
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

										if (ext.find("cj_traffic_light3") != std::string::npos || ext.find("cj_traffic_light4") != std::string::npos ||
											ext.find("cj_traffic_light5") != std::string::npos || ext.find("gay_traffic_light") != std::string::npos ||
											ext.find("mtraffic1") != std::string::npos || ext.find("mtraffic2") != std::string::npos ||
											ext.find("mtraffic4") != std::string::npos || ext.find("trafficlight1") != std::string::npos ||
											ext.find("vgsstriptlights1") != std::string::npos)
										{
											CoronaSize = 0.45f;
											LodLights << R << " " << G << " " << B << " " << offsetX << " " << offsetY << " " << offsetZ << " " << std::fixed << std::setprecision(2) << CoronaSize << std::setprecision(-1) << endl;
											break;
										}

										if (CoronaSize > 1.0f)
											CoronaSize = 1.2f;
										LodLights << R << " " << G << " " << B << " " << offsetX << " " << offsetY << " " << offsetZ << " " << std::fixed << std::setprecision(2) << CoronaSize << std::setprecision(-1) << endl;
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
	LodLights << "255 0 0 -9.298000 -11.402000 -68.624001 1.0 1" << endl;
	LodLights << "255 0 0 10.505000 -0.040000 -68.624001 1.0 1" << endl;
	LodLights << "255 0 0 -9.212000 11.244000 -68.624001 1.0 1" << endl;
	LodLights << "255 0 0 -7.926000 9.126000 -33.070000 1.0 1" << endl;
	LodLights << "255 0 0 7.932000 -0.002000 -33.070000 1.0 1" << endl;
	LodLights << "255 0 0 -7.974000 -9.203000 -33.070000 1.0 1" << endl;
	LodLights << "255 0 0 -6.840000 7.285000 0.826000 1.0 1" << endl;
	LodLights << "255 0 0 -6.812000 -7.408000 0.826000 1.0 1" << endl;
	LodLights << "255 0 0 5.768000 -0.097000 0.826000 1.0 1" << endl;
	LodLights << "255 0 0 7.377000 0.061000 24.687000 1.0 1" << endl;
	LodLights << "255 0 0 -7.586000 -8.599000 24.687000 1.0 1" << endl;
	LodLights << "255 0 0 -7.906000 8.648000 24.687000 1.0 1" << endl;
	LodLights << "255 0 0 -7.893000 15.860000 51.548000 1.0 1" << endl;
	LodLights << "255 0 0 -13.689000 12.518000 51.548000 1.0 1" << endl;
	LodLights << "255 0 0 13.900000 3.256000 51.797001 1.0 1" << endl;
	LodLights << "255 0 0 13.674000 -3.285000 51.797001 1.0 1" << endl;
	LodLights << "255 0 0 -7.933000 -15.856000 51.616001 1.0 1" << endl;
	LodLights << "255 0 0 -13.234000 -12.272000 51.548000 1.0 1" << endl;
	LodLights << "255 0 0 -8.114000 -9.256000 89.308998 1.0 1" << endl;
	LodLights << "255 0 0 8.037000 -0.015000 89.308998 1.0 1" << endl;
	LodLights << "%radarmast1_lawn" << endl;
	LodLights << "255 0 0 0.091 0.053 24.62 0.1 1" << endl;
	LodLights << "%radarmast1_lawn01" << endl;
	LodLights << "255 0 0 0.130000 -0.008000 24.490000 1.0 1" << endl;
	LodLights << "%ce_radarmast3" << endl;
	LodLights << "255 0 0 0.001 0.033 32.657 0.5 1" << endl;
	LodLights << "%ws_ref_bollard" << endl;
	LodLights << "255 0 0 0.009 0.001 0.469 0.7 1" << endl;
	LodLights << "%baybridge1_sfse" << endl;
	LodLights << "0 111 255 -76.679 -16.446 -8.721 1.2 1" << endl;
	LodLights << "0 111 255 -53.127 -16.434 -5.59 1.2 1" << endl;
	LodLights << "0 111 255 -29.32 -16.434 -0.143 1.2 1" << endl;
	LodLights << "0 111 255 -6.182 -16.434 7.343 1.2 1" << endl;
	LodLights << "0 111 255 16.348 -16.434 16.761 1.2 1" << endl;
	LodLights << "0 111 255 37.866 -16.434 28.076 1.2 1" << endl;
	LodLights << "0 111 255 58.783 -16.434 41.432 1.2 1" << endl;
	LodLights << "0 111 255 -76.679 16.468 -8.721 1.2 1" << endl;
	LodLights << "0 111 255 -53.127 16.468 -5.59 1.2 1" << endl;
	LodLights << "0 111 255 -29.32 16.49 -0.143 1.2 1" << endl;
	LodLights << "0 111 255 -6.182 16.49 7.343 1.2 1" << endl;
	LodLights << "0 111 255 16.348 16.49 16.761 1.2 1" << endl;
	LodLights << "0 111 255 37.866 16.49 28.076 1.2 1" << endl;
	LodLights << "0 111 255 58.783 16.49 41.432 1.2 1" << endl;
	LodLights << "%baybridge2_sfse" << endl;
	LodLights << "255 0 0 38.4845 -18.2165 83.4015 1.0 1" << endl;
	LodLights << "255 0 0 38.479 18.3384 83.4015 1.0 1" << endl;
	LodLights << "0 111 255 74.872 16.408 39.921 1.2 1" << endl;
	LodLights << "0 111 255 74.872 -16.476 39.921 1.2 1" << endl;
	LodLights << "0 111 255 56.636 -16.476 56.243 1.2 1" << endl;
	LodLights << "0 111 255 56.636 16.408 56.243 1.2 1" << endl;
	LodLights << "0 111 255 17.591 -16.476 53.324 1.2 1" << endl;
	LodLights << "0 111 255 17.591 16.408 53.324 1.2 1" << endl;
	LodLights << "0 111 255 -0.551 -16.476 37.625 1.2 1" << endl;
	LodLights << "0 111 255 -0.551 16.408 37.625 1.2 1" << endl;
	LodLights << "0 111 255 -19.959 -16.476 24.133 1.2 1" << endl;
	LodLights << "0 111 255 -19.959 16.408 24.133 1.2 1" << endl;
	LodLights << "0 111 255 -40.829 -16.476 12.153 1.2 1" << endl;
	LodLights << "0 111 255 -62.489 16.408 2.284 1.2 1" << endl;
	LodLights << "0 111 255 -62.489 -16.476 2.269 1.2 1" << endl;
	LodLights << "0 111 255 -84.947 -16.476 -5.715 1.2 1" << endl;
	LodLights << "0 111 255 -84.947 16.408 -5.701 1.2 1" << endl;
	LodLights << "%baybridge3_sfse" << endl;
	LodLights << "255 0 0 -100.83 18.3671 89.5265 1.0 1" << endl;
	LodLights << "255 0 0 -100.87 -18.1406 89.5265 1.0 1" << endl;
	LodLights << "0 111 255 136.609 -16.542 -5.215 1.2 1" << endl;
	LodLights << "0 111 255 136.609 16.383 -5.215 1.2 1" << endl;
	LodLights << "0 111 255 112.756 16.383 -8.19 1.2 1" << endl;
	LodLights << "0 111 255 112.756 -16.542 -8.19 1.2 1" << endl;
	LodLights << "0 111 255 69.149 16.383 -8.19 1.2 1" << endl;
	LodLights << "0 111 255 69.149 -16.542 -8.19 1.2 1" << endl;
	LodLights << "0 111 255 45.419 -16.542 -4.999 1.2 1" << endl;
	LodLights << "0 111 255 45.419 16.383 -4.999 1.2 1" << endl;
	LodLights << "0 111 255 22.189 -16.542 0.429 1.2 1" << endl;
	LodLights << "0 111 255 22.189 16.383 0.429 1.2 1" << endl;
	LodLights << "0 111 255 -0.257 -16.542 8.424 1.2 1" << endl;
	LodLights << "0 111 255 -0.257 16.383 8.424 1.2 1" << endl;
	LodLights << "0 111 255 -21.66 16.383 18.381 1.2 1" << endl;
	LodLights << "0 111 255 -21.66 -16.542 18.381 1.2 1" << endl;
	LodLights << "0 111 255 -42.686 -16.542 30.249 1.2 1" << endl;
	LodLights << "0 111 255 -42.686 16.383 30.249 1.2 1" << endl;
	LodLights << "0 111 255 -62.074 -16.421 43.898 1.2 1" << endl;
	LodLights << "0 111 255 -62.074 16.504 43.898 1.2 1" << endl;
	LodLights << "0 111 255 -80.451 -16.463 59.17 1.2 1" << endl;
	LodLights << "0 111 255 -80.451 16.462 59.17 1.2 1" << endl;
	LodLights << "%baybridge4_sfse" << endl;
	LodLights << "0 111 255 73.96 16.419 66.576 1.2 1" << endl;
	LodLights << "0 111 255 73.96 -16.496 66.576 1.2 1" << endl;
	LodLights << "0 111 255 55.538 16.419 50.292 1.2 1" << endl;
	LodLights << "0 111 255 55.538 -16.496 50.292 1.2 1" << endl;
	LodLights << "0 111 255 36.283 16.419 35.603 1.2 1" << endl;
	LodLights << "0 111 255 36.283 -16.496 35.603 1.2 1" << endl;
	LodLights << "0 111 255 15.391 16.419 22.126 1.2 1" << endl;
	LodLights << "0 111 255 15.391 -16.496 22.126 1.2 1" << endl;
	LodLights << "0 111 255 -6.209 16.419 10.828 1.2 1" << endl;
	LodLights << "0 111 255 -6.209 -16.496 10.828 1.2 1" << endl;
	LodLights << "0 111 255 -28.574 16.419 1.465 1.2 1" << endl;
	LodLights << "0 111 255 -28.574 -16.496 1.465 1.2 1" << endl;
	LodLights << "0 111 255 -51.905 16.419 -5.869 1.2 1" << endl;
	LodLights << "0 111 255 -51.905 -16.496 -5.869 1.2 1" << endl;
	LodLights << "0 111 255 -75.652 16.419 -11.285 1.2 1" << endl;
	LodLights << "0 111 255 -75.652 -16.496 -11.285 1.2 1" << endl;
	LodLights << "0 111 255 -99.5 16.419 -14.214 1.2 1" << endl;
	LodLights << "0 111 255 -99.5 -16.496 -14.214 1.2 1" << endl;
	LodLights << "%ggbrig_06_sfw" << endl;
	LodLights << "255 0 0 19.097 0.128 115.89 2.0 1" << endl;
	LodLights << "%ggbrig_01_sfw" << endl;
	LodLights << "255 0 0 19.097 0.128 115.89 2.0 1" << endl;
	LodLights << "%ggbrig_03_sfw" << endl;
	LodLights << "255 0 0 19.385 55.76 19.433 1.2 1" << endl;
	LodLights << "255 0 0 -19.136 55.76 19.433 1.2 1" << endl;
	LodLights << "255 0 0 -19.136 -5.989 -30.831 1.2 1" << endl;
	LodLights << "255 0 0 19.385 -5.989 -30.831 1.2 1" << endl;
	LodLights << "%ggbrig_05_sfw" << endl;
	LodLights << "255 0 0 -19.308 -59.193 17.728 1.2 1" << endl;
	LodLights << "255 0 0 19.136 -59.193 17.728 1.2 1" << endl;
	LodLights << "255 0 0 19.136 2.375 -31.452 1.2 1" << endl;
	LodLights << "255 0 0 -19.308 2.375 -31.452 1.2 1" << endl;
	LodLights << "%cxrf_whitebrig" << endl;
	LodLights << "249 145 34 -0.118 -29.064 5.175 1.2 1" << endl;
	LodLights << "249 145 34 16.632 -29.064 5.175 1.2 1" << endl;
	LodLights << "249 145 34 16.632 28.92 5.175 1.2 1" << endl;
	LodLights << "249 145 34 -0.118 28.92 5.175 1.2 1" << endl;
	LodLights << "%des_bigtelescope" << endl;
	LodLights << "255 0 0 24.051 24.258 53.915 3.0 1" << endl;
	LodLights << "%pylon_big1_" << endl;
	LodLights << "255 0 0 4.671 0.134 36.003 1.2 1" << endl;
	LodLights << "255 0 0 -4.356 0.134 36.003 1.2 1" << endl;

	LodLights << "%additional_coronas" << endl;
	LodLights << "255 255 255 -651.287 2145.91 74.7152 0.7 1" << endl;
	LodLights << "255 255 255 -781.256 2140.42 74.7152 0.7 1" << endl;
	LodLights << "255 255 255 -811.168 2015.78 57.1071 0.7 1" << endl;
	LodLights << "255 255 255 -794.209 2024.26 57.1071 0.7 1" << endl;
	LodLights << "255 255 255 -776.244 2031.43 57.1071 0.7 1" << endl;
	LodLights << "255 255 255 -733.823 2041.57 57.1071 0.7 1" << endl;
	LodLights << "255 255 255 -714.854 2043.3  57.1071 0.7 1" << endl;
	LodLights << "255 255 255 -695.276 2043.66 57.1071 0.7 1" << endl;
	LodLights << "255 255 255 -653.376 2038.31 57.1071 0.7 1" << endl;
	LodLights << "255 255 255 -635.249 2032.18 57.1071 0.7 1" << endl;
	LodLights << "255 255 255 -617.319 2024.71 57.1071 0.7 1" << endl;
	LodLights << "255 0 0 -750.894 2036.27 76.7071 0.7 1" << endl;
	LodLights << "255 0 0 -678.849 2040.47 76.7071 0.7 1" << endl;
	LodLights << "#place you coronas here" << endl;

	LodLights.close();
	SetCurrentDirectory("..\\");
}

int _tmain(int argc, _TCHAR* argv[])
{
	string line, line2;

	ifstream GTAData;
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
							if (sscanf(IDEline.c_str(), "%d %s %*s %*d %*s", &ID, &ModelName))
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
		LodLights << "#coronaColour.r, coronaColour.g, coronaColour.b, vecOffset.x, vecOffset.y, vecOffset.z, fCustomSize" << endl;
		LodLights << "#coronaColour2.r, coronaColour2.g, coronaColour2.b, vecOffset2.x, vecOffset2.y, vecOffset2.z, fCustomSize2" << endl;
		LodLights << "#%modelname2" << endl;
		LodLights << "#coronaColour.r, coronaColour.g, coronaColour.b, vecOffset.x, vecOffset.y, vecOffset.z, fCustomSize" << endl;
		LodLights << "#%additional_coronas" << endl;
		LodLights << "#coronaColour.r, coronaColour.g, coronaColour.b, position.x, position.y, position.z, fCustomSize, 1" << endl;
		LodLights << "#place you coronas here" << endl;
		LodLights << "#0.45 - reserved for traffic lights" << endl;
		LodLights << endl;

		static int PrevID;
		while (getline(FX2DData, line))
		{
			int ID; float offsetX, offsetY, offsetZ; int R, G, B; int unk1, type; string Corona, Shadow; float Distance, OuterRange, Size, InnerRange; int ShadowIntensity, Flash, Wet, Flare, Dust;
			line.erase(std::remove(line.begin(), line.end(), ','), line.end());
			if (sscanf(line.c_str(), "%d %f %f %f %d %d %d %d %d %s %s %f %f %f %f %d %d %d %d %d",
			&ID, &offsetX, &offsetY, &offsetZ, &R, &G, &B, &unk1, &type, &Corona, &Shadow, &Distance, &OuterRange, &Size, &InnerRange, &ShadowIntensity, &Flash, &Wet, &Flare, &Dust))
			{	
				if (Size > 1.0f)
					Size = 1.2f;

				if (!Size || ID == 4481 || ID == 4483) //destruct2 destruct03
					continue;

				if (PrevID != ID)
				{
					IDEData.clear();
					IDEData.seekg(0, ios::beg);
					while (getline(IDEData, line2))
					{
						line2.erase(std::remove(line2.begin(), line2.end(), ','), line2.end());
						int IdeID; string ModelName2;
						if (sscanf(line2.c_str(), "%d %s %*s %*d %*s", &IdeID, &ModelName2))
						{
							if (IdeID == ID)
							{
								LodLights << "%" << ModelName2.c_str() << endl;
								break;
							}
						}
					}
					std::transform(line2.begin(), line2.end(), line2.begin(), ::tolower);
					if (line2.find("mtraffic1") != std::string::npos || line2.find("mtraffic2") != std::string::npos ||
					line2.find("mtraffic4") != std::string::npos || line2.find("trafficlight1") != std::string::npos)
					{
						Size = 0.45f;
						LodLights << R << " " << G << " " << B << " " << offsetX << " " << offsetY << " " << offsetZ << " " << std::fixed << std::setprecision(2) << Size << std::setprecision(-1);
						PrevID = ID;
						continue;
					}

					LodLights << R << " " << G << " " << B << " " << offsetX << " " << offsetY << " " << offsetZ << " " << std::fixed << std::setprecision(2) << Size << std::setprecision(-1);
					PrevID = ID;
				}
				else
				{
					std::transform(line2.begin(), line2.end(), line2.begin(), ::tolower);
					if (!(line2.find("mtraffic1") != std::string::npos || line2.find("mtraffic2") != std::string::npos ||
						line2.find("mtraffic4") != std::string::npos || line2.find("trafficlight1") != std::string::npos))
					LodLights << R << " " << G << " " << B << " " << offsetX << " " << offsetY << " " << offsetZ << " " << std::fixed << std::setprecision(2) << Size << std::setprecision(-1);
				}
				if (line2.find("lamppost1") != std::string::npos || line2.find("lamppost2") != std::string::npos ||
					line2.find("lamppost3") != std::string::npos || line2.find("sub_floodlite") != std::string::npos ||
					line2.find("mlamppost") != std::string::npos || line2.find("doublestreetlght1") != std::string::npos ||
					line2.find("bollardlight") != std::string::npos || line2.find("lampost_coast") != std::string::npos)
				{
					LodLights << " " << "0" << " " << "1" << endl;
				}
				else
				{
					LodLights << endl;
				}
			}
		}
	}

	LodLights << "%additional_coronas" << endl;
	if (Filename == "IIILodLights.dat")
	{
			LodLights << "255 0 0 225.823 -1554.6 461.855 0.7 1" << endl;
			LodLights << "255 0 0 277.323 -1530.9 461.855 0.7 1" << endl;
			LodLights << "255 0 0 289.423 -1434.6 461.855 0.7 1" << endl;
			LodLights << "255 0 0 276.523 -1411.9 461.855 0.7 1" << endl;
			LodLights << "255 0 0 216.623 -1411.9 461.855 0.7 1" << endl;
			LodLights << "255 0 0 206.723 -1429.2 461.855 0.7 1" << endl;
			LodLights << "255 0 0 207.223 -1475.6 461.855 0.7 1" << endl;
			LodLights << "255 0 0 262.954 -1353.97 298.755 0.7 1" << endl;
			LodLights << "255 0 0 262.958 -1303.72 298.755 0.7 1" << endl;
			LodLights << "255 0 0 223.264 -1303.67 298.755 0.7 1" << endl;
			LodLights << "255 0 0 223.256 -1353.92 298.755 0.7 1" << endl;
			LodLights << "255 145 0 100.484 -1365 416.956 2.0 1" << endl;
			LodLights << "255 0 0 400.353 -328.015 187.855 1.0 1" << endl;
			LodLights << "255 0 0 26.6703 -1161.49 177.355 0.7 1" << endl;
			LodLights << "255 0 0 - 5.41379 -1146.4 176.955 0.7 1" << endl;
			LodLights << "255 0 0 - 39.8296 -1216 176.955 0.7 1" << endl;
			LodLights << "255 0 0 26.5036 -1247.63 177.355 0.7 1" << endl;
			LodLights << "255 255 255 460.366 -951.669 48.3555 1.2 1" << endl;
			LodLights << "255 255 255 460.366 -908.269 48.3555 1.2 1" << endl;
			LodLights << "255 255 255 486.166 -908.369 56.2555 1.2 1" << endl;
			LodLights << "255 255 255 486.166 -951.769 56.2555 1.2 1" << endl;
			LodLights << "255 255 255 509.166 -908.369 66.2555 1.2 1" << endl;
			LodLights << "255 255 255 509.166 -951.769 66.2555 1.2 1" << endl;
			LodLights << "255 255 255 531.566 -908.369 77.7555 1.2 1" << endl;
			LodLights << "255 255 255 531.566 -951.769 77.7555 1.2 1" << endl;
			LodLights << "255 255 255 554.466 -908.369 93.9555 1.2 1" << endl;
			LodLights << "255 255 255 554.466 -951.769 93.9555 1.2 1" << endl;
			LodLights << "255 255 255 577.366 -908.369 114.356 1.2 1" << endl;
			LodLights << "255 255 255 577.366 -951.769 114.356 1.2 1" << endl;
			LodLights << "255 255 255 599.566 -951.769 139.155 1.2 1" << endl;
			LodLights << "255 255 255 599.566 -908.369 139.155 1.2 1" << endl;
			LodLights << "255 255 255 771.866 -952.669 48.3555 1.2 1" << endl;
			LodLights << "255 255 255 771.366 -909.269 48.3555 1.2 1" << endl;
			LodLights << "255 255 255 744.966 -909.369 56.7555 1.2 1" << endl;
			LodLights << "255 255 255 745.466 -952.769 56.7555 1.2 1" << endl;
			LodLights << "255 255 255 722.666 -952.569 65.6555 1.2 1" << endl;
			LodLights << "255 255 255 722.166 -909.169 65.6555 1.2 1" << endl;
			LodLights << "255 255 255 700.166 -952.569 78.2555 1.2 1" << endl;
			LodLights << "255 255 255 699.666 -909.169 78.2555 1.2 1" << endl;
			LodLights << "255 255 255 678.066 -952.469 93.7555 1.2 1" << endl;
			LodLights << "255 255 255 677.566 -909.069 93.7555 1.2 1" << endl;
			LodLights << "255 255 255 654.466 -952.269 113.856 1.2 1" << endl;
			LodLights << "255 255 255 653.966 -908.869 113.855 1.2 1" << endl;
			LodLights << "255 255 255 632.466 -952.469 139.155 1.2 1" << endl;
			LodLights << "255 255 255 631.966 -909.069 139.155 1.2 1" << endl;
			LodLights << "#place you coronas here" << endl;
	}
	else
	{
			LodLights << "255 0 255 -682.821 -919.102 12.5548 0.7 1" << endl;
			LodLights << "255 0 255 -662.821 -920.202 13.7548 0.7 1" << endl;
			LodLights << "255 0 255 -642.821 -920.802 15.2548 0.7 1" << endl;
			LodLights << "255 0 255 -622.821 -921.002 16.8548 0.7 1" << endl;
			LodLights << "255 0 255 -682.821 -939.702 12.5548 0.7 1" << endl;
			LodLights << "255 0 255 -662.821 -940.602 13.7548 0.7 1" << endl;
			LodLights << "255 0 255 -642.821 -941.202 15.2548 0.7 1" << endl;
			LodLights << "255 0 255 -622.821 -941.402 16.8548 0.7 1" << endl;
			LodLights << "255 0 255 -602.821 -921.102 18.2548 0.7 1" << endl;
			LodLights << "255 0 255 -602.821 -941.502 18.2548 0.7 1" << endl;
			LodLights << "255 0 255 -582.821 -921.202 19.7548 0.7 1" << endl;
			LodLights << "255 0 255 -582.821 -941.602 19.7548 0.7 1" << endl;
			LodLights << "255 0 255 -562.821 -921.202 21.5548 0.7 1" << endl;
			LodLights << "255 0 255 -562.821 -941.602 21.5548 0.7 1" << endl;
			LodLights << "255 0 255 -542.821 -921.202 22.6548 0.7 1" << endl;
			LodLights << "255 0 255 -542.821 -941.602 22.6548 0.7 1" << endl;
			LodLights << "255 0 255 -522.821 -921.202 23.3548 0.7 1" << endl;
			LodLights << "255 0 255 -522.821 -941.602 23.3548 0.7 1" << endl;
			LodLights << "255 0 255 -502.821 -941.602 24.1548 0.7 1" << endl;
			LodLights << "255 0 255 -502.821 -921.202 24.1548 0.7 1" << endl;
			LodLights << "255 0 255 -482.821 -921.602 24.4548 0.7 1" << endl;
			LodLights << "255 0 255 -482.821 -942.002 24.4548 0.7 1" << endl;
			LodLights << "255 0 255 -462.821 -922.102 24.4548 0.7 1" << endl;
			LodLights << "255 0 255 -462.821 -942.502 24.4548 0.7 1" << endl;
			LodLights << "255 0 255 -442.821 -922.402 23.9548 0.7 1" << endl;
			LodLights << "255 0 255 -442.821 -942.802 23.9548 0.7 1" << endl;
			LodLights << "255 0 255 -422.821 -922.702 23.3548 0.7 1" << endl;
			LodLights << "255 0 255 -422.821 -943.102 23.3548 0.7 1" << endl;
			LodLights << "255 0 255 -402.821 -943.502 22.4548 0.7 1" << endl;
			LodLights << "255 0 255 -402.821 -923.102 22.4548 0.7 1" << endl;
			LodLights << "255 0 255 -382.821 -944.002 21.1548 0.7 1" << endl;
			LodLights << "255 0 255 -382.821 -923.602 21.1548 0.7 1" << endl;
			LodLights << "255 0 255 -362.821 -944.702 19.5548 0.7 1" << endl;
			LodLights << "255 0 255 -362.821 -924.302 19.5548 0.7 1" << endl;
			LodLights << "255 0 255 -342.821 -945.202 18.2548 0.7 1" << endl;
			LodLights << "255 0 255 -342.821 -924.802 18.2548 0.7 1" << endl;
			LodLights << "255 0 255 -322.821 -925.902 17.0548 0.7 1" << endl;
			LodLights << "255 0 255 -322.821 -946.402 17.0548 0.7 1" << endl;
			LodLights << "255 0 255 -302.821 -927.002 15.9548 0.7 1" << endl;
			LodLights << "255 0 255 -302.821 -947.502 15.9548 0.7 1" << endl;
			LodLights << "255 0 255 -282.821 -948.302 15.2548 0.7 1" << endl;
			LodLights << "255 0 255 -282.821 -927.902 15.2548 0.7 1" << endl;
			LodLights << "#place you coronas here" << endl;
	}

	LodLights.close();

	//std::cout << "Press ENTER to exit." << std::endl;
	//cin.ignore();
	return 0;
}

