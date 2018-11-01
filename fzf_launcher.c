#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h> 

// Pointers for directories and files
DIR *d;
FILE *fich;
struct dirent *dir;
// Location of system and user installed .desktop files
char systemdir[24] = "/usr/share/applications/";
char userdir[64];
// Current reading line on the looping .desktop file
char line[150];
// Full path to the .desktop file
char fullpath[100];
// List of app names piped into the program
char list[1024];
// Selected app name
char selection[100];
// Final command to execute through Sway IPC
char execute[150];
// Store for all apps names and their cmd
struct Applications {
	char name[100], exec[150];
};
// Temporal storage when ordering the array
char tempname[100], tempexec[150];;
// Pipe storage for subprocess
int childpipefd[2];
int parentpipefd[2];

int countfiles(char *directory) {
	// Total amount of .desktop files under the directory
	int amount = 0;
	// Open the directory
	d = opendir(directory);
	if (d) {
		// For each .desktop file increment the acknowledged amount by one
		while ((dir = readdir(d)) != NULL) {
			char *dot = strrchr(dir->d_name, '.');
			if (dot && !strcmp(dot, ".desktop")) {
				amount++;
			}
		}
		closedir(d);
	}
	return amount;
}

void fillstore(char *directory, struct Applications *apps) {
	// Open the systemdir to
	d = opendir(directory);
	if (d) {
		int i = 0;
		while ((dir = readdir(d)) != NULL) {
			char *dot = strrchr(dir->d_name, '.');
			if (dot && !strcmp(dot, ".desktop")) {
				// Build the fullpath based on the directory and the full name of the file
				strcpy(fullpath, directory);
				strcat(fullpath, dir->d_name);
				// Initialize some bools to stop the loop as soon as we find the first values
				_Bool name = 0, exec = 0;
				fich = fopen(fullpath, "r");
				while (fgets(line, sizeof(line), fich) && (!name || !exec)) {
					// If line starts with Name= save it on the store without the newline character
					if (strncmp(line, "Name=", 5) == 0) {
						name = 1;
						strcpy(apps[i].name, &line[5]);
					// If line starts with Exec= save it on the store without the newline character
					} else if (strncmp(line, "Exec=", 5) == 0) {
						exec = 1;
						char *ptr = strstr(line, "%");
						// Replace variables meant to be used for arguments from gui apps
						if (ptr) {
							// Position where the % is located
							int pos = ptr - line;
							char part1[150] = "", part2[150] = "";
							// Copy everything until that place
							strncpy(part1, line, pos-1);
							// Copy everything after that place+2 (to exclude the variable name too)
							strcpy(part2, &line[pos+2]);
							// Replace line with the two strings
							snprintf(line, sizeof(line), "%s%s", part1, part2);
						}
						strcpy(apps[i].exec, &line[5]);
					}
				}
				// Increment the counter to know where to place the next record in the store
				i++;
				fclose(fich);
			}
		}
		closedir(d);
	}
}

int main(int argc, char **argv) {
	// Retrieve amount of apps to store from each location
	int systemamount = countfiles(systemdir);
	snprintf(userdir, sizeof(userdir), "%s/.local/share/applications/", getenv("HOME"));
	int useramount = countfiles(userdir);
	// Create an array based on the Applications struct with the size required to fit all the apps
	struct Applications sapps[systemamount];
	struct Applications uapps[useramount];
	fillstore(systemdir, sapps);
	fillstore(userdir, uapps);
	// The final store should be able to contain both
	int totalamount = systemamount + useramount;
	struct Applications apps[totalamount];
	memcpy(apps, sapps, sizeof(sapps) + sizeof(uapps));
	// The array may end not fully filled if substitutions happened
	int leftovers = systemamount;
	// Merge stores by looping the user's (usually smaller) and replacing duplicated names
	for (int i = 0; i < useramount; i++) {
		_Bool replaced = 0;
		for (int j = 0; j < totalamount; j++) {
			if (!strcmp(apps[j].name, uapps[i].name)) {
				// Overwrite the system cmd with the user defined one
				strcpy(apps[j].exec, uapps[i].exec);
				replaced = 1;
				break;
			}
		}
		// If we didn't found a duplicated name we just append it to the end of the store
		if (!replaced) {
			strcpy(apps[leftovers].name, uapps[i].name);
			strcpy(apps[leftovers].exec, uapps[i].exec);
			leftovers++;
		}
	}
	// Arrange the store by alphabetical order
	for (int i = 0; i < leftovers; i++) {
		for (int j = i + 1; j < leftovers; j++) {
			// If the next name (by j index) is less that current (by i index) exchange positions
			if (strcmp(apps[i].name, apps[j].name) > 0) {
				// Of the names
				strcpy(tempname, apps[i].name);
				strcpy(apps[i].name, apps[j].name);
				strcpy(apps[j].name, tempname);
				// Of the cmds
				strcpy(tempexec, apps[i].exec);
				strcpy(apps[i].exec, apps[j].exec);
				strcpy(apps[j].exec, tempexec);
			}
		}
	}
	// Loop through the store and compose a newline separated list of program names
	for (int i = 0; i < leftovers; i++) {
		char program[150];
		snprintf(program, sizeof(program), "%s", apps[i].name);
		strcat(list, program);
	}
	// Show the selector to the user
	pipe(childpipefd);
	pipe(parentpipefd);
	if (fork() == 0) {
		// Child pipes
		dup2(parentpipefd[0], STDIN_FILENO);
		dup2(childpipefd[1], STDOUT_FILENO);
		// Execute fzf passing the received arguments as they come
		execv("/usr/bin/fzf", argv);
	}
	// Close unused pipes
	close(parentpipefd[0]);
	close(childpipefd[1]);
	// Send the list to stdin of program
	write(parentpipefd[1], list, strlen(list));
	// Retrieve the selection from stdout
	read(childpipefd[0], selection, sizeof(selection));
	// Loop the store and compose a command with the exec that matches the selected name
	for (int i = 0; i < leftovers; i++) {
		if (!strcmp(apps[i].name, selection)) {
			// Compose the message for swaymsg IPC socket (we may as well use i3-msg)
			snprintf(execute, sizeof(execute), "swaymsg -t command exec '%s'", apps[i].exec);
			break;
		}
	}
	// Finally launch the selected app
	system(execute);
	return(0);
}
