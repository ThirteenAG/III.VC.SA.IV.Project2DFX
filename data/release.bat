7za a -tzip ".\2DFXDataGrabber.zip" ".\2DFXDataGrabber\*"   -x!*.pdb -x!*.db -x!*.ipdb -x!*.iobj -x!*.lib -x!*.exp -x!.gitkeep
7za a -tzip ".\III.Project2DFX.zip" ".\IIILodLights\*"      -x!*.pdb -x!*.db -x!*.ipdb -x!*.iobj -x!*.lib -x!*.exp -x!.gitkeep
7za a -tzip ".\VC.Project2DFX.zip"  ".\VCLodLights\*"       -x!*.pdb -x!*.db -x!*.ipdb -x!*.iobj -x!*.lib -x!*.exp -x!.gitkeep
7za a -tzip ".\SA.Project2DFX.zip"  ".\SALodLights\*"       -x!*.pdb -x!*.db -x!*.ipdb -x!*.iobj -x!*.lib -x!*.exp -x!.gitkeep

7za x -p1 2DFXDataGrabber\iv_data.7z -o2DFXDataGrabber -y
copy /b/v/y "2DFXDataGrabber\2DFXDataGrabber.exe" "2DFXDataGrabber\iv_data\2DFXDataGrabber.exe"
cd 2DFXDataGrabber
cd iv_data
2DFXDataGrabber.exe
cd ..
cd ..
copy /b/v/y "2DFXDataGrabber\iv_data\IVLodLights.dat" "IVLodLights\IVLodLights.dat"

7za a -tzip ".\IV.Project2DFX.zip"  ".\IVLodLights\*"       -x!*.pdb -x!*.db -x!*.ipdb -x!*.iobj -x!*.lib -x!*.exp -x!.gitkeep
EXIT

7-Zip Extra
~~~~~~~~~~~
License for use and distribution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Copyright (C) 1999-2016 Igor Pavlov.

7-Zip Extra files are under the GNU LGPL license.


Notes: 
  You can use 7-Zip Extra on any computer, including a computer in a commercial 
  organization. You don't need to register or pay for 7-Zip.


GNU LGPL information
--------------------

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You can receive a copy of the GNU Lesser General Public License from 
  http://www.gnu.org/

