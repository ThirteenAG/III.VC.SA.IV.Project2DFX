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
					ofstream out2DFX(ext, fstream::trunc | ios::binary);
					string buf;
					while (!DFFContent.eof())
					{
						getline(DFFContent, buf);

						std::size_t found = buf.find("coronastar");
						if (found != std::string::npos)
						{
							found -= 49;
							out2DFX.write(buf.c_str() + found, buf.size() - found);
							break;
						}

					}
					DFFContent.close();
					out2DFX.close();
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

		LodLights << "%additional_coronas" << endl;
		LodLights << "255 255 255 - 651.287 2145.91 74.7152 0.7 1" << endl;
		LodLights << "255 255 255 - 781.256 2140.42 74.7152 0.7 1" << endl;
		LodLights << "255 255 255 - 811.168 2015.78 57.1071 0.7 1" << endl;
		LodLights << "255 255 255 - 794.209 2024.26 57.1071 0.7 1" << endl;
		LodLights << "255 255 255 - 776.244 2031.43 57.1071 0.7 1" << endl;
		LodLights << "255 255 255 - 733.823 2041.57 57.1071 0.7 1" << endl;
		LodLights << "255 255 255 - 714.854 2043.3  57.1071 0.7 1" << endl;
		LodLights << "255 255 255 - 695.276 2043.66 57.1071 0.7 1" << endl;
		LodLights << "255 255 255 - 653.376 2038.31 57.1071 0.7 1" << endl;
		LodLights << "255 255 255 - 635.249 2032.18 57.1071 0.7 1" << endl;
		LodLights << "255 255 255 - 617.319 2024.71 57.1071 0.7 1" << endl;
		LodLights << "255 0 0 - 750.894 2036.27 76.7071 0.7 1" << endl;
		LodLights << "255 0 0 - 678.849 2040.47 76.7071 0.7 1" << endl;
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
		int PrevID;
		while (getline(FX2DData, line))
		{
			int ID; float offsetX, offsetY, offsetZ; int R, G, B; int unk1, type; string Corona, Shadow; float Distance, OuterRange, Size, InnerRange; int ShadowIntensity, Flash, Wet, Flare, Dust;
			line.erase(std::remove(line.begin(), line.end(), ','), line.end());
			if (sscanf(line.c_str(), "%d %f %f %f %d %d %d %d %d %s %s %f %f %f %f %d %d %d %d %d",
			&ID, &offsetX, &offsetY, &offsetZ, &R, &G, &B, &unk1, &type, &Corona, &Shadow, &Distance, &OuterRange, &Size, &InnerRange, &ShadowIntensity, &Flash, &Wet, &Flare, &Dust))
			{	
				if (Size > 1.0f)
					Size = 1.2f;

				if (!Size)
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
						LodLights << R << " " << G << " " << B << " " << offsetX << " " << offsetY << " " << offsetZ << " " << std::fixed << std::setprecision(2) << Size << std::setprecision(-1) << endl;
						PrevID = ID;
						continue;
					}

					LodLights << R << " " << G << " " << B << " " << offsetX << " " << offsetY << " " << offsetZ << " " << std::fixed << std::setprecision(2) << Size << std::setprecision(-1) << endl;
					PrevID = ID;
				}
				else
				{
					if (!(line2.find("mtraffic1") != std::string::npos || line2.find("mtraffic2") != std::string::npos ||
						line2.find("mtraffic4") != std::string::npos || line2.find("trafficlight1") != std::string::npos))
					LodLights << R << " " << G << " " << B << " " << offsetX << " " << offsetY << " " << offsetZ << " " << std::fixed << std::setprecision(2) << Size << std::setprecision(-1) << endl;
				}
			}
		}
	}

	LodLights << "%additional_coronas" << endl;
	if (Filename == "IIILodLights.dat")
	{
			LodLights << "255 0 0 225.823 - 1554.6 461.855 0.7 1" << endl;
			LodLights << "255 0 0 277.323 - 1530.9 461.855 0.7 1" << endl;
			LodLights << "255 0 0 289.423 - 1434.6 461.855 0.7 1" << endl;
			LodLights << "255 0 0 276.523 - 1411.9 461.855 0.7 1" << endl;
			LodLights << "255 0 0 216.623 - 1411.9 461.855 0.7 1" << endl;
			LodLights << "255 0 0 206.723 - 1429.2 461.855 0.7 1" << endl;
			LodLights << "255 0 0 207.223 - 1475.6 461.855 0.7 1" << endl;
			LodLights << "255 0 0 262.954 - 1353.97 298.755 0.7 1" << endl;
			LodLights << "255 0 0 262.958 - 1303.72 298.755 0.7 1" << endl;
			LodLights << "255 0 0 223.264 - 1303.67 298.755 0.7 1" << endl;
			LodLights << "255 0 0 223.256 - 1353.92 298.755 0.7 1" << endl;
			LodLights << "255 145 0 100.484 - 1365 416.956 2.0 1" << endl;
			LodLights << "255 0 0 400.353 - 328.015 187.855 1.0 1" << endl;
			LodLights << "255 0 0 26.6703 - 1161.49 177.355 0.7 1" << endl;
			LodLights << "255 0 0 - 5.41379 - 1146.4 176.955 0.7 1" << endl;
			LodLights << "255 0 0 - 39.8296 - 1216 176.955 0.7 1" << endl;
			LodLights << "255 0 0 26.5036 - 1247.63 177.355 0.7 1" << endl;
			LodLights << "255 255 255 460.366 - 951.669 48.3555 1.2 1" << endl;
			LodLights << "255 255 255 460.366 - 908.269 48.3555 1.2 1" << endl;
			LodLights << "255 255 255 486.166 - 908.369 56.2555 1.2 1" << endl;
			LodLights << "255 255 255 486.166 - 951.769 56.2555 1.2 1" << endl;
			LodLights << "255 255 255 509.166 - 908.369 66.2555 1.2 1" << endl;
			LodLights << "255 255 255 509.166 - 951.769 66.2555 1.2 1" << endl;
			LodLights << "255 255 255 531.566 - 908.369 77.7555 1.2 1" << endl;
			LodLights << "255 255 255 531.566 - 951.769 77.7555 1.2 1" << endl;
			LodLights << "255 255 255 554.466 - 908.369 93.9555 1.2 1" << endl;
			LodLights << "255 255 255 554.466 - 951.769 93.9555 1.2 1" << endl;
			LodLights << "255 255 255 577.366 - 908.369 114.356 1.2 1" << endl;
			LodLights << "255 255 255 577.366 - 951.769 114.356 1.2 1" << endl;
			LodLights << "255 255 255 599.566 - 951.769 139.155 1.2 1" << endl;
			LodLights << "255 255 255 599.566 - 908.369 139.155 1.2 1" << endl;
			LodLights << "255 255 255 771.866 - 952.669 48.3555 1.2 1" << endl;
			LodLights << "255 255 255 771.366 - 909.269 48.3555 1.2 1" << endl;
			LodLights << "255 255 255 744.966 - 909.369 56.7555 1.2 1" << endl;
			LodLights << "255 255 255 745.466 - 952.769 56.7555 1.2 1" << endl;
			LodLights << "255 255 255 722.666 - 952.569 65.6555 1.2 1" << endl;
			LodLights << "255 255 255 722.166 - 909.169 65.6555 1.2 1" << endl;
			LodLights << "255 255 255 700.166 - 952.569 78.2555 1.2 1" << endl;
			LodLights << "255 255 255 699.666 - 909.169 78.2555 1.2 1" << endl;
			LodLights << "255 255 255 678.066 - 952.469 93.7555 1.2 1" << endl;
			LodLights << "255 255 255 677.566 - 909.069 93.7555 1.2 1" << endl;
			LodLights << "255 255 255 654.466 - 952.269 113.856 1.2 1" << endl;
			LodLights << "255 255 255 653.966 - 908.869 113.855 1.2 1" << endl;
			LodLights << "255 255 255 632.466 - 952.469 139.155 1.2 1" << endl;
			LodLights << "255 255 255 631.966 - 909.069 139.155 1.2 1" << endl;
			LodLights << "#place you coronas here" << endl;
	}
	else
	{
			LodLights << "255 0 255 - 682.821 - 919.102 12.5548 0.7 1" << endl;
			LodLights << "255 0 255 - 662.821 - 920.202 13.7548 0.7 1" << endl;
			LodLights << "255 0 255 - 642.821 - 920.802 15.2548 0.7 1" << endl;
			LodLights << "255 0 255 - 622.821 - 921.002 16.8548 0.7 1" << endl;
			LodLights << "255 0 255 - 682.821 - 939.702 12.5548 0.7 1" << endl;
			LodLights << "255 0 255 - 662.821 - 940.602 13.7548 0.7 1" << endl;
			LodLights << "255 0 255 - 642.821 - 941.202 15.2548 0.7 1" << endl;
			LodLights << "255 0 255 - 622.821 - 941.402 16.8548 0.7 1" << endl;
			LodLights << "255 0 255 - 602.821 - 921.102 18.2548 0.7 1" << endl;
			LodLights << "255 0 255 - 602.821 - 941.502 18.2548 0.7 1" << endl;
			LodLights << "255 0 255 - 582.821 - 921.202 19.7548 0.7 1" << endl;
			LodLights << "255 0 255 - 582.821 - 941.602 19.7548 0.7 1" << endl;
			LodLights << "255 0 255 - 562.821 - 921.202 21.5548 0.7 1" << endl;
			LodLights << "255 0 255 - 562.821 - 941.602 21.5548 0.7 1" << endl;
			LodLights << "255 0 255 - 542.821 - 921.202 22.6548 0.7 1" << endl;
			LodLights << "255 0 255 - 542.821 - 941.602 22.6548 0.7 1" << endl;
			LodLights << "255 0 255 - 522.821 - 921.202 23.3548 0.7 1" << endl;
			LodLights << "255 0 255 - 522.821 - 941.602 23.3548 0.7 1" << endl;
			LodLights << "255 0 255 - 502.821 - 941.602 24.1548 0.7 1" << endl;
			LodLights << "255 0 255 - 502.821 - 921.202 24.1548 0.7 1" << endl;
			LodLights << "255 0 255 - 482.821 - 921.602 24.4548 0.7 1" << endl;
			LodLights << "255 0 255 - 482.821 - 942.002 24.4548 0.7 1" << endl;
			LodLights << "255 0 255 - 462.821 - 922.102 24.4548 0.7 1" << endl;
			LodLights << "255 0 255 - 462.821 - 942.502 24.4548 0.7 1" << endl;
			LodLights << "255 0 255 - 442.821 - 922.402 23.9548 0.7 1" << endl;
			LodLights << "255 0 255 - 442.821 - 942.802 23.9548 0.7 1" << endl;
			LodLights << "255 0 255 - 422.821 - 922.702 23.3548 0.7 1" << endl;
			LodLights << "255 0 255 - 422.821 - 943.102 23.3548 0.7 1" << endl;
			LodLights << "255 0 255 - 402.821 - 943.502 22.4548 0.7 1" << endl;
			LodLights << "255 0 255 - 402.821 - 923.102 22.4548 0.7 1" << endl;
			LodLights << "255 0 255 - 382.821 - 944.002 21.1548 0.7 1" << endl;
			LodLights << "255 0 255 - 382.821 - 923.602 21.1548 0.7 1" << endl;
			LodLights << "255 0 255 - 362.821 - 944.702 19.5548 0.7 1" << endl;
			LodLights << "255 0 255 - 362.821 - 924.302 19.5548 0.7 1" << endl;
			LodLights << "255 0 255 - 342.821 - 945.202 18.2548 0.7 1" << endl;
			LodLights << "255 0 255 - 342.821 - 924.802 18.2548 0.7 1" << endl;
			LodLights << "255 0 255 - 322.821 - 925.902 17.0548 0.7 1" << endl;
			LodLights << "255 0 255 - 322.821 - 946.402 17.0548 0.7 1" << endl;
			LodLights << "255 0 255 - 302.821 - 927.002 15.9548 0.7 1" << endl;
			LodLights << "255 0 255 - 302.821 - 947.502 15.9548 0.7 1" << endl;
			LodLights << "255 0 255 - 282.821 - 948.302 15.2548 0.7 1" << endl;
			LodLights << "255 0 255 - 282.821 - 927.902 15.2548 0.7 1" << endl;
			LodLights << "#place you coronas here" << endl;
	}

	LodLights.close();

	//std::cout << "Press ENTER to exit." << std::endl;
	//cin.ignore();
	return 0;
}

