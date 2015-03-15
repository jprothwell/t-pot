# Installation Guide #

## Requirements ##

  * T-PoT is a plug-in for Total Commander, which only runs on **Windows**. It has been tested with Windows XP Home SP2 and Windows XP Professional SP2, but there is no reason it shouldn't work with Vista, the only issue probably being the next requirement.

  * **Total Commander**. It was mainly tested with version 7.02a, but it should be back-compatible with version 6. You can download it from [Total Commander home page](http://www.ghisler.com/), it's a 30-day trial but really worth the buy - even without the T-PoT plug-in ;-)

  * **iTunes** 7.4 or later, that you can download from [the iTunes download page](http://www.apple.com/itunes/download/).

## Adding the Plug-in to Total Commander ##

**1. Download the latest version of the T-PoT plug-in**

You can get it from [this page](http://code.google.com/p/t-pot/downloads/list). There are two versions:
  * one compiled with Visual Studio 2005 and which requires [Microsoft .NET Framework Version 2.0](http://www.microsoft.com/downloads/details.aspx?FamilyID=0856EACB-4362-4B0D-8EDD-AAB15C5E04F5&displaylang=en).
  * one compiled with Visual Studio 2003 that only requires msvcp71.dll and msvcr71.dll (which I put on the download page as [Libs2003.zip](http://t-pot.googlecode.com/files/Libs2003.zip). You can copy those files into C:\Windows\System32.

If you are not sure, choose the VS2003, it offers exactly the same features and the libraries are easier to get than the other version. Most up-to-date Windows systems should have the necessary libraries for the VS2005 version though.

**2. Save the zip file in a temporary directory and open it with Total Commander.**

Enter the following address in your favourite browser: http://code.google.com/p/t-pot/downloads/list

> ![http://scythal.googlepages.com/Inst_01.png](http://scythal.googlepages.com/Inst_01.png)

Left-click on the version you want to install, and select the directory you want to save it to (some browsers will automatically download the file at a pre-defined location):

> ![http://scythal.googlepages.com/Inst_02.png](http://scythal.googlepages.com/Inst_02.png)

_Note: If that opens Winzip instead, do a right-click on the link and choose "Save link as..." instead (or save the zip file from Winzip itself)._

**3. Adding the Plug-in.**

Go to the download directory with Total Commander. To open the zip file, select the file and press Enter or double-click it:

> ![http://scythal.googlepages.com/Inst_03.png](http://scythal.googlepages.com/Inst_03.png)

Total Commander will recognize a self-installable plug-in and ask whether you want to install it or not:

> ![http://scythal.googlepages.com/Inst_04.png](http://scythal.googlepages.com/Inst_04.png)

Simply click Yes.

If you miss some of the libraries mentioned earlier, you will get an error message:

> ![http://scythal.googlepages.com/Inst_04err.png](http://scythal.googlepages.com/Inst_04err.png)

Otherwise you can proceed with the installation.

  * If this is the first time you install the plug-in, Total Commander will ask you where to store the files. You can safely use the default path:

> ![http://scythal.googlepages.com/Inst_05a.png](http://scythal.googlepages.com/Inst_05a.png)

  * If there was another version already installed, Total Commander will aks whether you want to overwrite it, or install the plug-in elsewhere. Normally, you should overwrite the previous version:

> ![http://scythal.googlepages.com/Inst_05b.png](http://scythal.googlepages.com/Inst_05b.png)

_Note: the default path may change from one version of Total Commander to the next. For example, 7.01 proposes_

&lt;TC&gt;

\plugins\wfx\T-PoT, whereas 7.02a has 

&lt;TC&gt;

\plugins\T-Pot as a default path. You needn't worry, the actual directory doesn't matter at all.

**4. Installation Status.**

Once the plug-in is installed, the _File system plugins_ window opens and shows you the installed plug-ins. You should see a T-PoT entry:

> ![http://scythal.googlepages.com/Inst_06.png](http://scythal.googlepages.com/Inst_06.png)

You can rename it if you like, this will only change the name seen in the Network Neighborhood as explained below. From that window, you can also remove the plug-in. This will only unload it from Total Commander, the files will be left in the installation directory.

You can get back to the _File system plugins_ window anytime from the _Configuration_ menu, by choosing _Options_, then _Plugins_. Click on the _Configure_ button of the _File system plugins (.WFX)_ category to open the window.

**5. Check if the plug-in is visible.**

Open the Network Neighborhood in Total Commander:
  * open the left or right drive letter drop-down (alternatively, press Alt-F1 or Alt-F2)
  * select "[\-] Network Neighborhood" at the bottom of the list

You should see a [T-PoT] entry, or the name you renamed the plug-in to, next to an iPod icon:

> ![http://scythal.googlepages.com/Inst_07.png](http://scythal.googlepages.com/Inst_07.png)

**6. Browse the iPod directories.**

First, make sure the iPod is connected to the USB port of your computer and that it is recognized by iTunes.

Enter the [T-PoT] directory by selecting it with the arrow keys and pressing Enter, or by double-clicking it with the left mouse button:

> ![http://scythal.googlepages.com/Inst_08.png](http://scythal.googlepages.com/Inst_08.png)

Note that the last screenshot above shows what you should see on a jailbroken iPod. In its original configuration, you will have the /var/root/Media as root directory and won't be able to see the whole file system.

That's it, you're in the iPod, congratulations!

# Next Step #

[Using T-PoT](UserGuide.md).