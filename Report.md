# ECS150 W20, Project 1 Report

## Authors
[Deng, Julie] (914732600)

[Wu, Andy] (914649586)

## High-Level Design Choices
At the high level, we implemented a simple shell to handle various commands. Our shell executes user-supplied commands with optional arguments and composes commands via piping.

The primary data structures we used are structs to represent commands. This way, it was easy to keep track of commands from the parent process and to write the functions that execute these commands.

## Implementation Details
### Parsing
First, we implement a parse function dedicated to figuring out if there is a pipeline command or not, and if so, how many such commands there are. We also check in parsing to see if we must redirect output. If so, we redirect stdout to a specified file name given by user input. If there is only one command, we parse only that one command and store it into a struct, then pass it into a function to execute the command. However, if there is more than one pipe, then we clone a child process from the shell and then parse the piped commands.

Lastly, we restore stdout so that we can print to terminal again. This only occurs after we redirect output.

### Commands
To implement our command line, we tokenized the commands that the user passed in and then store that in a struct. While we tokenize the commands, we check for errors. After that, we pass the member variables of the struct as an argument into the exec function.

We implemented the built in commands on our own, such as cd, pwd, and the exit function. First, while parsing, we check to see if the command is one of the aforementioned, and then if so, we call them directly with our own implementation.

## Testing Our Project
In order to test our project, we used the reference shell provided by Professor Porquet and his team. This proved to be the most helpful in testing our program.

We used the given test shell and the sample commands to further test our program. In addition, we also tested using commands that we believed we may also be test on, such as certain edge cases regarding error management.

## Sources
1. Lecture Notes
2. Documentation
