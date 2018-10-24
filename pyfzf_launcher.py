#!/usr/bin/env python3
import os
import sys
import re
import subprocess

def main():
    # List of directories to search on
    directories = [
        "/usr/share/applications/",
        os.getenv("HOME") + "/.local/share/applications/"
    ]
    # Store for each app name and command
    apps = {}

    # Build an fzf command based on the options passed as arguments
    command = ["fzf"]
    for i, arg in enumerate(sys.argv):
        if i > 0:
            command.append(arg)

    # Loop through every dir to retrieve needed information
    for dir in directories:
        desktops = os.listdir(dir)
        # Only read the file if it is a .desktop
        for file in (file for file in desktops if ".desktop" in file):
            with open(dir + file, "r") as file:
                name = None
                exec = None
                for line in file:
                    # We do only take the two first definitions for Name and Exec
                    if re.search("^Name=", line):
                        name = line.replace("Name=", "").strip()
                    elif re.search("^Exec=", line):
                        # Variables are only used to pass arguments when calling from other apps
                        exec = re.sub(" %.+?(?=( |$))", "", line.replace("Exec=", "").strip())
                    # Exit loop if we already found both variables
                    if name and exec:
                        break
                # Add the new entry to the list
                apps[name] = exec
    # Use alphabetical order and create a newline separated string from the list
    list = "\n".join(sorted(apps.keys()))
    # Enter selection mode
    fzf = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    # Retrieve selection and pass it to sway's IPC socket for execution
    election = fzf.communicate(input=list.encode())[0].decode().strip()
    subprocess.call(["swaymsg", "-t", "command", "exec", apps[election]])
    sys.exit(0)
