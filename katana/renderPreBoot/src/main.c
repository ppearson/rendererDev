/*
 RenderPreBoot
 Created by Peter Pearson in 2018.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Note: explicitly written in C so that dependencies are minimal, and we can build with any compiler we want
//       and not have to mess around with libstdc++ lib in LD_LIBRARY_PATH, which will likely cause issues with
//       Katana / Renderboot itself if the compiler versions aren't compatible...

// Note: we can only print to stderr within renderboot fork. Anything else doesn't get printed to the render log...

// use main() version with env variables so we can just pass stuff directly through to execve().
// the use of execve() means the real renderboot keeps our PID, although it does mean we can't run any code ourselves
// after the real renderboot finishes...
int main(int argc, char** argv, char** envp)
{
	char* katanaRoot = getenv("KATANA_ROOT");
	if (!katanaRoot)
	{
		fprintf(stderr, "RenderPreBoot: Error - KATANA_ROOT env variable is not set, so cannot find where the real renderboot is.\n\n");
		return -1;
	}
	
	fprintf(stderr, "Invoking RenderPreBoot...\n");
	
	char* extraCommands = getenv("RPB_EXTRA_COMMANDS");
	
	int numArgs = argc;
	int numExtraCommands = 0;
		
	char realRenderbootPath[2048];
	memset(realRenderbootPath, 0, 2048);
	strcpy(realRenderbootPath, katanaRoot);
	strcat(realRenderbootPath, "/bin/renderboot");
	
	int nextArg = 0;
	int argOffset = 0;
	
	if (extraCommands && extraCommands[0] != '\0')
	{
		// we're going to stick some extra commands in front of renderboot, so count how many we have first...
		const char* extraTmp = extraCommands;
		while (*extraTmp)
		{
			if (*extraTmp++ == '|')
			{
				numExtraCommands++;
			}
		}
		
		numExtraCommands += 1;		
		
		numArgs += numExtraCommands;
		nextArg += numExtraCommands;
		argOffset = numExtraCommands;
	}

	char** newTargetArgs = (char**)malloc((numArgs + 1) * sizeof(char*));
	if (!newTargetArgs)
	{
		fprintf(stderr, "RenderPreBoot: Can't allocate memory.\n\n");
		return -1;
	}
	
	if (numExtraCommands == 0)
	{
		// if we don't have extra commands, the first arg is just renderboot as normal
		newTargetArgs[0] = realRenderbootPath;
	}
	else
	{
		fprintf(stderr, "RenderPreBoot: invoking pre command: %s\n\n", extraCommands);
		
		// if we did have extra commands, fill them in here...
		if (numExtraCommands == 1)
		{
			// there's only one
			newTargetArgs[0] = extraCommands;
		}
		else
		{
			int commandCount = 0;
			char* commandToken = strtok(extraCommands, "|");
			for (; commandToken; commandToken = strtok(NULL, "|"))
			{
				int length = strlen(commandToken);
				
				char* newCommand = (char*)malloc(length + 1);
				strcpy(newCommand, commandToken);
				newTargetArgs[0 + commandCount++] = newCommand;
			}
		}
		
		// now the actual real renderboot
		newTargetArgs[nextArg] = realRenderbootPath;
	}
	
	nextArg++;
	
	// then the remainder of the arguments from the original invocation of us
	for (int i = nextArg; i < numArgs; i++)
	{
		newTargetArgs[i] = argv[i - argOffset];
	}
	newTargetArgs[numArgs] = NULL;
	
#if 0
	for (int i = 0; i < numArgs; i++)
	{
		fprintf(stderr, "%s\n", newTargetArgs[i]);
	}
	return 0;
#endif
	
	// Note: as this effectively replaces our process, it doesn't really return (assuming it succeeds),
	//       so we can't really free the memory we allocated above, and have to "leak" it...
	
	// TODO: it might be worth having the option to call something else so that we can run code after here,
	//       but that might require handling the environment vars differently...
	
	return execve(newTargetArgs[0], newTargetArgs, envp);
}
