# FOP Project (Vim)
## About me
**Name: Sahand Akramipour**
<br>
Student No.: 401110618

## Releases
### [v1.0.0](https://github.com/FundamentalOfProgramming-SUT-2022/project-shndap/releases/tag/v1.0.0)
Only [Phase 1](https://github.com/FundamentalOfProgramming-SUT-2022/Project/blob/main/Phase1/Phase1.pdf) is developed.
- CLI based
- No graphics
- Commands:
  - ```createfile```: Create file
  - ```insertstr```: Insert
  - ```cat```: Cat
  - ```removestr```: Remove
  - ```copystr```: Copy  (built-in clipboard)
  - ```cutstr```: Cut   (built-in clipboard)
  - ```pastestr```: Paste (built-in clipboard)
  - ```find```: Find
  - ```replace```: Replace
  - ```grep```: Grep
  - ```undo```: Undo
  - ```auto-indent```: Closing pairs
  - ```compare```: Text comparator
  - ```tree```: Directory tree
  - ```=D```: Arman
  

### [v2.0.0](https://github.com/FundamentalOfProgramming-SUT-2022/project-shndap/releases/tag/v2.0.0)
[Phase 2](https://github.com/FundamentalOfProgramming-SUT-2022/Project/blob/main/Phase2/Phase2.pdf) developed.
- Graph interface added
#### Commands
- Mode manipulation commands:
	- ```ESC (key)```:		Switch to NORMAL (CLI) mode
	- ```!VISUAL```:	(In NORMAL mode) Switches to VISUAL
	- ```!INSERT```:	(In NORMAL mode) Switches to INSERT

- Cursor commands:
	- ```h```:	Left
	- ```j```: 	Down
	- ```k```:	Up
	-	```l```: Right

- Shortcuts:
	- ```/<exp>```:	(Normal mode)	Find all occurences of ```<exp>```
	- ```n```:		(After ```/<exp>```)	Go to next occurence of ```<exp>```
	- ```=```:		(Normal mode) 	Auto-Indent
	- ```u```:		(Normal mode)	Undo
	- ```y```:		(Visual mode)	Copy
	- ```t```:		(Visual mode)	Cut
	- ```d```:		(Visual mode)	Delete
	- ```p```:	(Visual mode)	Paste
	- ```ctrl+b```:	(Insert mode)	Start of text (other shortcuts and navigation buttons will not work until ```ctrl+c``` is pressed)
	- ```ctrl+c```:	(Insert mode)	End of text (No text will be inserted/deleted and only shortcut or navigation buttons will work until ```ctrl+b``` is pressed)
  -	```ctrl+x```:	Exit

- Phase 1 commands:
  - You can use phase 1 commands by inserting a ```:``` in the begginning of command
