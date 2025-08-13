# MapImageGenerator
## A bmp map generator for Cultures series games
Example output:\
<img width="400" height="400" alt="ExampleImage" src="https://github.com/user-attachments/assets/ab7a0f98-6c65-4e5c-a45d-f52c2c606462" />\
The output image can be loaded into the cultures map editor when creating a new map.\
<img width="281" height="406" alt="importExample" src="https://github.com/user-attachments/assets/b041bcbd-65b2-4293-9209-a144a260449d" />\
The app supports changing generation paramaters such as seed, values at which land and mountains begin, temperature and humidity values and whether to generate beaches and lava.
## Usage
### Creating a project
First you will need to create a new project by going to File -> New project ...\
Enter the name of your project, and set the desired width and height of your map. The map dimensions cannot exceed 500x500.\
After creating your project a new map with a random seed will be generated
### Seed
The seed of the map is a 24 digit number;
* First 6 digits are responsible for the height map
* Digits 6-12 affect the temperature map
* Digits 12-18 affect the humidity map
* Remaining 6 digits are used to generate the detail map
<!-- end of the list -->
You can generate a new map with a random seed by pressing "Generate with random seed" button.\
You can also enter your own 24 digit seed which will be used if you press the "Generate" button located next to the seed textbox.
### Editing your project
Various parameters can be changed by using sliders or updown controls in the window to the right of the displayed map.\
To the right of the controls window is a color palette of the displayed map.
### Saving a project
You can save your current project by going to File -> Save or File -> Save as ...\
When you choose a location, it will be used again if you go to File -> Save.\
If you want a different location, you should use File -> Save as ...
### Opening a project
You can open a project by going to File -> Open ...\
If you have previously created and saved a project it will be available in the recent files list below the Export button in the File menu.\
After choosing a file, the saved map will be recreated.
### Exporting a project
If you go to File -> Export or simply click the "Export" button under the seed field, you will be prompted to choose a location for the .bmp file.\
The saved .bmp file can be then used in the cultures game map editor.
### Language settings
By default the app will use system language if it's supported. Currently supported languages are English and Polish, but this list will hopefully be expanded in the future.\
You can change the default language by going to Help -> Lanugage and choosing your preferred language.
## Installation
Download and run.\
It is recommended to put the executable in it's own folder as it creates an ini file.\
Tested to work on Windows 7, 10, and 11.