
# pyfzf-launcher
This is a very simple python wrapper around fzf to pick and launch an app through swaywm (or i3) IPC features.

## How?
It retrieves all the information it needs by parsing .desktop files from ***/usr/share/applications/*** and then from ***$HOME/.local/share/applications*** for an easier way of overwritting default values.

## Installation
1. Clone this repository
```
git clone https://github.com/eskuero/pyfzf_launcher
```
2. Install with pip after entering the newly created folder
```
pip3 install --user .
```
## Usage
Launch with the command
```
pyfzf-launcher
```
Since it's meant as an app launcher is recommended to use it in combination with a terminal emulator window floating in the screen. You can pass to it any argument that fzf would accept.
Per example:

```
konsole
	--profile "Pop-up" \
	-e pyfzf-launcher \
		-i \
		--layout=reverse \
		--prompt "Launch: " \
		--color="bg+:#073642,bg:#002b36,spinner:#719e07,hl:#586e75"
```
Will open a konsole window, with all the settings from the profile "Pop-up" and execute the launcher enabling case insensitive search, arranging the options by alphabetical order, using the string "Launch: " as custom prompt and a set of colors that will look like this:

<img src="https://raw.githubusercontent.com/eskuero/pyfzf-launcher/master/example.gif" width=640>

