/*
	Circle Pad example made by Aurelio Mannara for ctrulib
	Please refer to https://github.com/smealum/ctrulib/blob/master/libctru/include/3ds/services/hid.h for more information
	This code was modified for the last time on: 12/13/2014 2:20 UTC+1

	This wouldn't be possible without the amazing work done by:
	-Smealum
	-fincs
	-WinterMute
	-yellows8
	-plutoo
	-mtheall
	-Many others who worked on 3DS and I'm surely forgetting about
*/

/*
Credit to LiquidFenrir for file browsing code.
Special thanks to @Neos4Smash for testing certain features

Need help? Make an issue. I will be glad to help.

Think the code is garbage? Be thankful it's open source unlike a certain other launcher!
(Also make a pull request if you have improvements)

Planned Features: Load custom options from a modpack.txt file for simple file replacement?
Syntax:
Night/Day Title Screen,Day Screen,Night Screen,/ui/lumen/title_up/img-00001.tex,/ui/lumen/title_up/title_day.tex,/ui/lumen/title_up/title_night.tex
Explanation: 1st section is option name, 2nd is 1st option name, 3rd is 2nd option name (So it will say "%s is enabled"),4th is normal file path,5th is path of disabled item when 2nd option is chosen, 6th is path of disabled item when 1st option is chosen.

The license of this code is as follows:
- Please credit RhythmLunatic if you want to use this
- You may NOT distribute the unmodified source code and sell it for profit.
- Other than that, do whatever you want with it. Give it out, turn it into your own launcher, anything your heart desires.
*/

//#include "common.h"
//#include "dir.h"
//#include "draw.h"
#include <3ds.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
//#include <filesystem>

//Terminal colors.
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"


//Because fuck you that's why
//using std::string;
//using std::cout;

/*I think this is recommended against due
  to the fact that it pollutes the namespace, so be aware
  of that...
  I'm no pro C++ programmer, so I don't know for sure.
  */
using namespace std;


/*
This defines if an option is available.
Meaning:
0: The option is available.
1: The csv was formatted incorrectly.
2: The main file needed for this option is missing (part 3 in the csv). 
3: There are two alt files. There should only be one alt file at a time.
4. There is no alt file to choose from.
5. There are no files!
*/
enum State
{
	WORKING,
	BAD_CSV,
	MAIN_FILE_MISSING,
	TWO_ALT_FILES,
	NO_ALT_FILE,
	NO_FILES
};

const string stDescriptions[6] = {
	"The option is working correctly.",
	"The CSV is incomplete or unreadable.",
	"The main file is missing.",
	"There are two alt files, and no main file.",
	"There is no alternate file to choose from.",
	"There are no files for this option. (Check your path)"
};

//This stores everything related to an option.
class Option
{
	public:
		string name;		//name of option
		string enabledName; //What to show when it's enabled
		string disabledName;//What to show when it's disabled
		string mainPath;	//main path of file
		string altPath1;	//path of alt file
		string altPath2;	//Path of main file when alt file is being used
		State availibility;	//If this option is available and usable.
		bool enabled;		//Stores if an alternate setting is toggled off or on
};

bool InitSettings();
bool readFile();
const vector<string> explode(const string& s, const char& c);
void printDebug(string clr, string str);
bool fileOrFolderExists(string pathString);
const vector<string> getDirectories(string path);
bool applySettings();
string enumToString(State st);

ofstream myfile;
const string rootDir = "/SaltySD/";
const string defaultDir = "smash";
bool settingsFileExists = false;
vector<string> smashFolders;
int selectedDir = 0;
string desc = "";


//It's an array of options, obviously.
vector<Option> optArray;


//Init top and bottom screens for console
PrintConsole ct;
PrintConsole cb;

int position = 0;

//The help strings.
const string help1 = "\x1b[0;0HPress Start to exit and DPad to navigate.";
const string help2 = "\x1b[1;0HPress X or Y to switch between modpacks (Your changes will be lost!).";
const string help3 = "\x1b[2;0HPress Select to apply settings and launch the currently selected modpack.\n";

int main(int argc, char **argv)
{
	// Initialize services
	gfxInitDefault();

	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
    consoleInit(GFX_TOP, &ct);
    consoleInit(GFX_BOTTOM, &cb);
	consoleSelect(&ct);
	
	//Matrix containing the name of each key. Useful for printing when a key is pressed
	char keysNames[32][32] = {
		"KEY_A", "KEY_B", "KEY_SELECT", "KEY_START",
		"KEY_DRIGHT", "KEY_DLEFT", "KEY_DUP", "KEY_DDOWN",
		"KEY_R", "KEY_L", "KEY_X", "KEY_Y",
		"", "", "KEY_ZL", "KEY_ZR",
		"", "", "", "",
		"KEY_TOUCH", "", "", "",
		"KEY_CSTICK_RIGHT", "KEY_CSTICK_LEFT", "KEY_CSTICK_UP", "KEY_CSTICK_DOWN",
		"KEY_CPAD_RIGHT", "KEY_CPAD_LEFT", "KEY_CPAD_UP", "KEY_CPAD_DOWN"
	};
	
	u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame

	//The basic of basic checks
	if (!fileOrFolderExists(rootDir + "smash"))
	{
		printDebug(RED, "Hey genius, you need a smash folder to use smash mods.");
	}
	
	smashFolders = getDirectories(rootDir);
	for (unsigned int i = 0; i < smashFolders.size(); i++)
	{
		//printDebug(WHT, smashFolders[i]);
		//crappy C directory scan might find the folders out of order, so set the current directory to the one that's currently being used manually.
		string s = "smash";
		if (s.compare(smashFolders[i]) == 0)
		{
			selectedDir = i;
			printDebug(WHT, "set the current dir int to " + to_string(i));
		}
	}
	
	InitSettings();
	
	//I'm not sure why the top screen is red, so here's the lazy fix.
	cout << RESET;
	consoleSelect(&ct);
	
	//Draw all the info text
	std::cout << help1;
	std::cout << help2;
	std::cout << help3;
	std::cout << help4;
	cout << "\x1b[4;0HCurrently selected modpack: " << desc << "\n";
	
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();
		//hidKeysHeld returns information about which buttons have are held down in this frame
		u32 kHeld = hidKeysHeld();
		//hidKeysUp returns information about which buttons have been just released
		u32 kUp = hidKeysUp();

		if (kDown & KEY_START) break; // break in order to return to hbmenu

		//Do the keys printing only if keys have changed
		if (kDown != kDownOld || kHeld != kHeldOld || kUp != kUpOld)
		{
			//Clear console
			consoleClear();

			//These two lines must be rewritten because we cleared the whole console
			std::cout << help1;
			std::cout << help2;
			std::cout << help3;
			cout << "\x1b[4;0HCurrently selected modpack: " << desc << "\n";
			
			
			//Check if some of the keys are down, held or up
			int i;
			for (i = 0; i < 32; i++)
			{
				//Debugging information.
				if (kDown & BIT(i)) printf("%s down (Key %i)\n", keysNames[i], i);
				if (kHeld & BIT(i)) printf("%s held (Key %i)\n", keysNames[i], i);
				if (kUp & BIT(i)) printf("%s up\n", keysNames[i]);

				
				if ((kDown & BIT(i)) && i == 11) //11 = KEY_Y
				{
					if (selectedDir > 0)
						selectedDir--;
					else
						selectedDir = smashFolders.size() - 1;

					InitSettings();
				}
				if ((kDown & BIT(i)) && i == 10) //10 = KEY_X
				{
					if (selectedDir < smashFolders.size() - 1)
						selectedDir++;
					else
						selectedDir = 0;

					InitSettings();
				}
				
				if (settingsFileExists)
				{
					//Actual menu code is here. This increments and decrements the position (duh)
					if ((kDown & BIT(i)) && i == 6) //6 = KEY_UP	
					{
						if (position > 0)
							position--;
						else
							position = optArray.size() - 1;
					}
					if ((kDown & BIT(i)) && i == 7) //7 = KEY_DOWN
					{
						if (position < optArray.size() - 1)
							position++;
						else
							position = 0;
					}

					//Press left to turn something off, press right to turn it on.
					if ((kDown & BIT(i)) && i == 5 && optArray[position].availibility == WORKING) //5 = KEY_LEFT
					{
						optArray[position].enabled = false;
					}
					if ((kDown & BIT(i)) && i == 4 && optArray[position].availibility == WORKING) //4 = KEY_RIGHT
					{
						optArray[position].enabled = true;
					}
				}
		
				if ((kDown & BIT(i)) && i == 2) //2 == KEY_SELECT
				{
					consoleSelect(&cb);
					consoleClear();
					applySettings();
					consoleSelect(&ct);
		    
				}
		
		
			}

		}

		int lineStart = 10;
		if (settingsFileExists)
		{
			for (int i = 0; i < optArray.size(); i++)
			{
				//Draw a ">" to the left of the currently selected option, else draw a space and the option name
				//It's too much effort to figure out why to_string(lineStart + i) crashes the program so use
				//printf instead.
				if (position == i)
					printf("\x1b[%i;0H>%s", lineStart + i, optArray[i].name.c_str());
				else
					printf("\x1b[%i;0H %s", lineStart + i, optArray[i].name.c_str());

			}
			if (optArray[position].availibility == WORKING)
			{
				cout << "\x1b[20;0HPress left or right to choose the " << optArray[position].name << ".";
				cout << "\x1b[21;0H< " << (optArray[position].enabled ? optArray[position].enabledName : optArray[position].disabledName) << " >"; //When option = true (enabled), display the first string, else display the second one.
			}
			else
			{
				cout << RED << "\x1b[20;0HThis option is unavailable. Error: " << enumToString(optArray[position].availibility) << RESET;
			}	
		}
		else
		{
			printf("\x1b[%i;0H%s", lineStart, "This modpack doesn't have a settings file.");
		}
		//Set keys old values for the next frame
		kDownOld = kDown;
		kHeldOld = kHeld;
		kUpOld = kUp;

		//circlePosition pos;

		//Read the CirclePad position
		//hidCircleRead(&pos);

		//Print the CirclePad position
		//printf("\x1b[2;0H%04d; %04d", pos.dx, pos.dy);
		if (settingsFileExists)
			cout << "\x1b[9;0HPosition: " << to_string(position);
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}
	
	// Exit services
	gfxExit();
	return 0;
	
};

//The function that sets up your settings!
bool InitSettings()
{
	string currentDir = rootDir + smashFolders[selectedDir];
	printDebug(WHT, "Setting current directory to " + currentDir);
	chdir(currentDir.c_str());
	string line;
	ifstream myfile("desc.txt");
	if (myfile.is_open())
	{
		printDebug(GRN, "Opened desc.txt successfully.");
		getline(myfile,line);
		desc = line;
	}
	else
	{
		desc = smashFolders[selectedDir] + " (no desc.txt found)";
	}
	
	//Read modpack.csv from file.
	settingsFileExists = readFile();
	//Debugging information.
	consoleSelect(&cb);
	
	if (settingsFileExists)
	{	
		cout << WHT << "Got an array of " << optArray.size() << ". \n";
		printDebug(WHT,"Testing 1st option readibility.");
		//optArray[0].name = "THIS IS A TEST";
		printDebug(WHT, optArray[0].name);
		/*for (unsigned int i = 0; i < optArray.size(); i++)
		{
			printDebug(WHT,optArray[i].name);
		}*/	
	}
	else
	{
		printDebug(WHT, "Skipping array test...");
	}
	
}

bool applySettings()
{
	if (!settingsFileExists)
		return false;
	
	for (int i = 0; i < optArray.size(); i++)
	{
		if (optArray[i].availibility == WORKING)
		{
			Option option = optArray[i];
			if (fileOrFolderExists(option.altPath2) && option.enabled)
			{
				//const char * oldName = option.altPath2.c_str();
				//const char * newName = 
				//First, rename the file currently being used to its name when not being used
				if (rename(option.mainPath.c_str(), option.altPath1.c_str()) == 0)
				{
					//Next, rename the alt file not being used to main
					if (rename(option.altPath2.c_str(), option.mainPath.c_str()) != 0)
					{	
						printDebug(RED, "Couldn't rename " + option.mainPath + " to " + option.altPath2);
					}
					else
					{
						printDebug(GRN, "Option " + option.name + " applied successfully.");
					}
				}
				else
				{
					printDebug(RED, "Couldn't rename " + option.mainPath + " to " + option.altPath1);
				}
				
			}
			else if (fileOrFolderExists(option.altPath1) && option.enabled == false)
			{
				if (rename(option.mainPath.c_str(), option.altPath2.c_str()) == 0)
				{
					if (rename(option.altPath1.c_str(), option.mainPath.c_str()) != 0)
					{	
						printDebug(RED, "Couldn't rename " + option.mainPath + " to " + option.altPath1);
					}
					else
					{
						printDebug(GRN, "Option " + option.name + " applied successfully.");
					}
				}
				else
				{
					printDebug(RED, "Couldn't rename " + option.mainPath + " to " + option.altPath2);
				}
				
			}
			else
			{
				printDebug(WHT, "No changes made to " + option.name);
			}
			
		}
	}
	InitSettings();
	
}

bool moveSmashFolders()
{
	string currentDir = rootDir + smashFolders[selectedDir];
	const string dd = rootDir + defaultDir;
	if (currentDir == dd)
	{
		printDebug(WHT, "No need to move smash folders.");
		return false;
	}
	else
	{
		string newDir = "";
		bool foundDirName = false;
		int i = 1;
		while (foundDirName == false)
		{
			//Keep going until it finds a directory name that doesn't exist.
			//So smash1, smash2, smash3, smash4, etc...
			if (fileOrFolderExists(rootDir + defaultDir + to_string(i)))
			{
				i++;
			}
			else
			{
				newDir = rootDir + defaultDir + to_string(i);
				foundDirName = true;
			}
		}
		//This might take a while!
		if (rename(dd.c_str(), newDir.c_str()) == 0)
		{
			if (rename(currentDir.c_str(), dd.c_str()) == 0)
				return true;
			else
			{
				printDebug(RED, "Something went wrong when trying to rename " + currentDir + " to " + dd);
			}
		}
		else //This else statement isn't needed, but hey, readable code..
		{
			printDebug(RED, "something went wrong when trying to rename " + dd + " to " + newDir);
		}
		return false;
	}
}

//Should be renamed to "readcsv"
bool readFile() {
	string line;
	ifstream myfile("settings.csv");
	if (myfile.is_open())
	{
		consoleSelect(&cb);
		//cout << GRN << "Opened txt file successfully.";
		printDebug(GRN,"Opened csv file successfully.");
		consoleSelect(&ct);
		optArray.clear(); //If another modpack had options...
		
		while ( getline (myfile,line) )
		{
			//cout << line << '\n';
			//printDebug(WHT, line);

			Option option;
			vector<string> optArr = explode(line, ',');

			if (optArr.size() != 6)
			{
				option.availibility = BAD_CSV;
				printDebug(RED, "The line in the CSV is bad!");
				printDebug(WHT, line);
			}
			else
				option.availibility = WORKING;
			/*
			string name;		//name of option
			string enabledName; //What to show when it's enabled
			string disabledName;//What to show when it's disabled
			string mainPath;	//main path of file
			string altPath1;	//path of alt file
			string altPath2;	//Path of main file when alt file is being used
			*/
			option.name = optArr[0];
			option.disabledName = optArr[1];
			option.enabledName = optArr[2];
			option.mainPath = optArr[3];
			option.altPath1 = optArr[4];
			option.altPath2 = optArr[5];
			//If a an alt item is enabled, the default one (ex: title_day, or altPath1) would exist
			//while the alt one (altPath2) doesn't (since it's currently being used)
			if (fileOrFolderExists(option.altPath1))
			{
				printDebug(WHT, option.name + " is ON.");
				option.enabled = true;	
			}
			else if (fileOrFolderExists(option.altPath2))
			{
				printDebug(WHT, option.name + " is OFF.");
				option.enabled = false;
			}
			else if (!fileOrFolderExists(option.mainPath))
			{
				option.enabled = false;
				option.availibility = MAIN_FILE_MISSING;
				printDebug(RED, enumToString(option.availibility));
				printDebug(RED, "Tested path " + option.mainPath);
			}
			else
			{
				option.enabled = false;
				option.availibility = NO_ALT_FILE;
				printDebug(RED, enumToString(option.availibility));
				/*printDebug(RED, "Tested path " + option.mainPath);
				printDebug(RED, "And path " + option.altPath1);
				printDebug(RED, "And path " + option.altPath2);*/
			}

			//If two alt files...
			if (fileOrFolderExists(option.altPath1) && fileOrFolderExists(option.altPath2))
			{
				option.availibility = TWO_ALT_FILES;
			};
			optArray.push_back(option);
		};
		myfile.close();
		printDebug(WHT, "csv closed.");
	}
	//else cout << "Unable to open file"; 
	else
	{
		printDebug(RED,"Unable to open CSV!");
		return false;
	};
	
	if (optArray.size() == 0)
	{
		printDebug(RED, "CSV had no lines or incorrect line endings!");
		return false;
	}
	return true;
}

/*
Adapted from http://www.cplusplus.com/articles/2wA0RXSz/
This function will generate and return an Option class by parsing
a string and separating it by a char.
In this case, it's separating a line from a csv and separating it by
comma to generate an option...
*/
const vector<string> explode(const string& s, const char& c)
{
    string buff{""};
    vector<string> v;
	
    for(auto n:s)
    {
        if(n != c) buff+=n;
		else
        	if(n == c && buff != "")
			{
				v.push_back(buff);
				buff = "";
			}
	}
    if(buff != "") v.push_back(buff);

	
    return v;
};

//Prints to the bottom screen, obviously.
void printDebug(string clr,string str)
{
	consoleSelect(&cb);
	cout << clr << str << "\n";
	consoleSelect(&ct);
};

/*int initializeSettings(Vector<string> option)
{
	printDebug(WHT,"Checking option " + " (" + to_string(i) + "/" + to_string(optArray.size()) + ")");
};*/

/*There's no need to differentiate between a file or folder since rename
will work regardless, it just needs to exist */
bool fileOrFolderExists(string pathString)
{
	struct stat s;
	const char * path = pathString.c_str();
	consoleSelect(&cb);
	//printf("%s%s", "\n", path);
	consoleSelect(&ct);
	if (stat(path,&s) == 0)
	{
		if( s.st_mode & S_IFDIR ) //If dir
		{
			return true;
		}
		else if( s.st_mode & S_IFREG ) //If file
		{
			return true;
		}
	}
	return false;
};

const vector<string> getDirectories(string path)
{
	DIR *dir;
	struct dirent *ent;
	vector <string> v;
	
	if ((dir = opendir(path.c_str())) != NULL) { //If path isn't null
		while ((ent = readdir(dir)) != NULL) { //While reading directory...
			if (ent->d_type == DT_DIR) //Check if directory. (use DT_REG for files.)
				v.push_back(ent->d_name); //Add to vector
			//strcpy(dirInfoArray[count].name, ent->d_name); //Copy filenames into the dirInfoArray
			//dirInfoArray[count].isFile = (ent->d_type == 8); //check if file.
			//count++;
		}
		closedir (dir);
	}
	
	return v;	
}


string enumToString(State st)
{
	return stDescriptions[st];
	
}
